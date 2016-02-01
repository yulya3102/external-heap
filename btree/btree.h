#pragma once

#include <vector>
#include <map>
#include <cassert>
#include <algorithm>

#include <boost/optional.hpp>

#include "../storage/memory.h"

namespace detail
{
template <typename Key, typename Value>
struct b_internal;

template <typename Key, typename Value>
struct b_leaf;

template <typename Key, typename Value>
struct b_node : std::enable_shared_from_this<b_node<Key, Value> >
{
    using b_node_ptr = std::shared_ptr<b_node>;
    using b_internal_ptr = std::shared_ptr<b_internal<Key, Value> >;
    using b_leaf_ptr = std::shared_ptr<b_leaf<Key, Value> >;

    virtual std::size_t size() const = 0;
    virtual b_node * copy(const storage::memory<b_node> & storage) const = 0;
    virtual void reload() = 0;

    storage::memory<b_node> & storage_;
    storage::node_id id_;
    boost::optional<storage::node_id> parent_;

    b_node(storage::memory<b_node> & storage)
        : storage_(storage)
        , id_(storage.new_node())
        , parent_(boost::none)
    {}

    b_node(const b_node & other, storage::memory<b_node> & storage)
        : storage_(storage)
        , id_(other.id_)
        , parent_(other.parent_)
    {}

    virtual ~b_node() = default;

    b_internal_ptr load_parent() const
    {
        if (parent_)
            return std::dynamic_pointer_cast<b_internal<Key, Value> >(storage_.load_node(*parent_));
        return nullptr;
    }

    // Split node: replace the node with its parent (or new internal node)
    // make new node (right brother) of the same type as the node
    // and make the node and its new brother children of the new internal node
    // Then return new parent of the node
    virtual b_node_ptr split_full(size_t t, boost::optional<storage::node_id> & tree_root) = 0;

    // Make correct links from (maybe new) parent to the node and its new right brother
    // and update all parent links and root link if necessary
    void update_parent(b_internal_ptr parent, b_node_ptr new_brother, boost::optional<storage::node_id> & tree_root)
    {
        // Replace link from parent to old child with two links:
        // to old child and to new child
        // or just add two new links if parent is empty
        {
            auto it = std::find(parent->children_.begin(), parent->children_.end(), id_);
            // *it == this || it == end
            if (it != parent->children_.end())
                it = parent->children_.erase(it);
            // *it == element after this
            // insert before it
            it = parent->children_.insert(it, new_brother->id_);
            // *it == new_brother
            // insert before it
            it = parent->children_.insert(it, id_);
            // *it == this
        }

        // Update root link if necessary
        if (!this->parent_)
        {
            tree_root = parent->id_;
            parent->parent_ = boost::none;
        }

        // Update old child and new child parent links
        this->parent_ = parent->id_;
        new_brother->parent_ = parent->id_;
    }

    virtual void add(Key && key, Value && value, size_t t, boost::optional<storage::node_id> & tree_root) = 0;
};

template <typename Key, typename Value>
struct b_leaf : b_node<Key, Value>
{
    using b_internal_ptr = typename b_node<Key, Value>::b_internal_ptr;
    using b_node_ptr = typename b_node<Key, Value>::b_node_ptr;
    using b_leaf_ptr = typename b_node<Key, Value>::b_leaf_ptr;

    std::vector<std::pair<Key, Value> > values_;

    b_leaf(storage::memory<b_node<Key, Value> > & storage)
        : b_node<Key, Value>(storage)
    {}

    b_leaf(const b_leaf & other, storage::memory<b_node<Key, Value> > & storage)
        : b_node<Key, Value>(other, storage)
        , values_(other.values_)
    {}

    virtual std::size_t size() const
    {
        return values_.size();
    }

    virtual b_leaf * copy(const storage::memory<b_node<Key, Value> > & storage) const
    {
        return new b_leaf(*this, const_cast<storage::memory<b_node<Key, Value> > &>(storage));
    }

    virtual void reload()
    {
        b_leaf_ptr updated_this = std::dynamic_pointer_cast<b_leaf>(this->storage_.load_node(this->id_));
        this->values_ = updated_this->values_;
        this->id_ = updated_this->id_;
        this->parent_ = updated_this->parent_;
    }

    virtual ~b_leaf() = default;

    b_node_ptr split_full(size_t t, boost::optional<storage::node_id> & tree_root)
    {
        assert(this->size() == 2 * t - 1);

        b_internal_ptr parent = this->load_parent();

        assert(!parent || parent->size() < 2 * t - 1);

        if (!parent)
            parent = std::make_shared<b_internal<Key, Value> >(this->storage_);
        b_leaf_ptr brother = std::make_shared<b_leaf<Key, Value> >(this->storage_);

        auto split_by_it = this->values_.begin();
        for (size_t i = 0; i < t - 1; ++i)
            ++split_by_it;

        {
            // Insert new key after one pointing to x
            auto this_it = std::find(parent->children_.begin(), parent->children_.end(), this->id_);
            size_t this_i = this_it - parent->children_.begin();
            auto this_key_it = parent->keys_.begin() + this_i;
            parent->keys_.insert(this_key_it, split_by_it->first);
        }

        for (auto it = split_by_it; it != this->values_.end(); ++it)
            brother->values_.push_back(std::move(*it));
        this->values_.erase(split_by_it, this->values_.end());

        // Make correct links from parent to the node and its new brother
        // and update all parent links
        this->update_parent(parent, brother, tree_root);

        this->storage_.write_node(brother->id_, brother.get());

        return parent;
    }

    virtual void add(Key && key, Value && value, size_t t, boost::optional<storage::node_id> & tree_root)
    {
        if (size() == 2 * t - 1)
        {
            b_node_ptr next = split_full(t, tree_root);

            this->storage_.write_node(this->id_, this);

            next->add(std::move(key), std::move(value), t, tree_root);

            this->reload();

            this->storage_.write_node(next->id_, next.get());
        }
        else
        {
            auto v = std::make_pair(std::move(key), std::move(value));
            auto it = std::lower_bound(this->values_.begin(), this->values_.end(), v);
            this->values_.insert(it, std::move(v));
        }
    }
};

template <typename Key, typename Value>
struct b_internal : b_node<Key, Value>
{
    using b_internal_ptr = typename b_node<Key, Value>::b_internal_ptr;
    using b_node_ptr = typename b_node<Key, Value>::b_node_ptr;

    std::vector<Key> keys_;
    std::vector<storage::node_id> children_;

    b_internal(storage::memory<b_node<Key, Value> > & storage)
        : b_node<Key, Value>(storage)
    {}

    b_internal(const b_internal & other, storage::memory<b_node<Key, Value> > & storage)
        : b_node<Key, Value>(other, storage)
        , keys_(other.keys_)
        , children_(other.children_)
    {}

    virtual std::size_t size() const
    {
        return keys_.size();
    }

    virtual b_internal * copy(const storage::memory<b_node<Key, Value> > & storage) const
    {
        return new b_internal(*this, const_cast<storage::memory<b_node<Key, Value> > &>(storage));
    }

    virtual void reload()
    {
        b_internal_ptr updated_this = std::dynamic_pointer_cast<b_internal>(this->storage_.load_node(this->id_));
        this->children_ = updated_this->children_;
        this->keys_ = updated_this->keys_;
        this->id_ = updated_this->id_;
        this->parent_ = updated_this->parent_;
    }

    virtual ~b_internal() = default;

    b_node_ptr split_full(size_t t, boost::optional<storage::node_id> & tree_root)
    {
        assert(this->size() == 2 * t - 1);

        b_internal_ptr parent = this->load_parent();

        assert(!parent || parent->size() < 2 * t - 1);

        if (!parent)
            parent = std::make_shared<b_internal>(this->storage_);
        b_internal_ptr brother = std::make_shared<b_internal>(this->storage_);

        auto split_keys = this->keys_.begin() + (t - 1);
        auto split_children = this->children_.begin() + t;
        auto it = std::find(parent->children_.begin(), parent->children_.end(), this->id_);
        auto i = it - parent->children_.begin();
        auto i_key = parent->keys_.begin() + i;

        parent->keys_.insert(i_key, std::move(*split_keys));

        for (auto it_keys = split_keys + 1; it_keys != this->keys_.end(); ++it_keys)
            brother->keys_.push_back(std::move(*it_keys));

        for (auto it_children = split_children; it_children != this->children_.end(); ++it_children)
        {
            brother->children_.push_back(std::move(*it_children));
            b_node_ptr child = this->storage_.load_node(brother->children_.back());
            child->parent_ = brother->id_;
            this->storage_.write_node(child->id_, child.get());
        }

        this->keys_.erase(split_keys, this->keys_.end());
        this->children_.erase(split_children, this->children_.end());

        // Make correct links from parent to the node and its new brother
        // and update all parent links
        this->update_parent(parent, brother, tree_root);

        this->storage_.write_node(brother->id_, brother.get());

        return parent;
    }

    virtual void add(Key && key, Value && value, size_t t, boost::optional<storage::node_id> & tree_root)
    {
        assert(std::is_sorted(keys_.begin(), keys_.end()));

        if (size() == 2 * t - 1)
        {
            b_node_ptr next = split_full(t, tree_root);

            /*
             * Outdated nodes: after function call, every node
             * except function parameters and return value
             * is outdated and should be reloaded from storage
             */
            // 'this' will be outdated after next->add
            // so I'll need to reload it
            // but it can be not in storage yet, so I need to write it
            this->storage_.write_node(this->id_, this);

            next->add(std::move(key), std::move(value), t, tree_root);

            this->reload();

            this->storage_.write_node(next->id_, next.get());
        }
        else
        {
            auto it = std::lower_bound(keys_.begin(), keys_.end(), key);
            std::size_t i = it - keys_.begin();
            b_node_ptr child = this->storage_.load_node(children_[i]);

            this->storage_.write_node(this->id_, this);

            child->add(std::move(key), std::move(value), t, tree_root);

            this->reload();

            this->storage_.write_node(child->id_, child.get());
        }
    }

    void merge_with_right_brother(std::size_t i, b_internal_ptr right_brother, std::size_t t, boost::optional<storage::node_id> & tree_root)
    {
        assert(this->parent_ == right_brother->parent_);
        b_internal_ptr parent = this->load_parent();

        assert(parent->children_[i] == this->id_);
        assert(!parent->parent_ || parent->size() > t - 1);

        // Move children from right brother to the node
        for (auto child_it = right_brother->children_.begin();
             child_it != right_brother->children_.end();
             ++child_it)
        {
            b_node_ptr child = this->storage_.load_node(*child_it);
            child->parent_ = this->id_;
            this->storage_.write_node(child->id_, child.get());
            this->children_.push_back(std::move(*child_it));
        }

        // Move key from parent to the node
        this->keys_.push_back(std::move(parent->keys_[i]));
        parent->keys_.erase(parent->keys_.begin() + i);

        // Move keys from right brother to the node
        for (auto key_it = right_brother->keys_.begin();
             key_it != right_brother->keys_.end();
             ++key_it)
            this->keys_.push_back(std::move(*key_it));

        // Remove link to right brother from parent
        parent->children_.erase(parent->children_.begin() + i + 1);

        // If parent became empty (could happen only if it is root and had only one key)
        // then make the node new root
        if (parent->size() == 0)
        {
            tree_root = this->id_;
            this->storage_.delete_node(parent->id_);
            this->parent_ = boost::none;
        }
        else
            this->storage_.write_node(parent->id_, parent.get());
    }
};
}

namespace bptree
{
template <typename Key, typename Value, int t>
struct b_tree
{
    void add(Key key, Value value)
    {
        b_node_ptr root = load_root();
        root->add(std::move(key), std::move(value), t, root_);
        nodes_.write_node(root->id_, root.get());
    }

    template <typename OutIter>
    OutIter remove_left_leaf(OutIter out)
    {
        b_node_ptr node = load_root();

        while (!is_leaf(node))
        {
            b_internal_ptr node_int = std::dynamic_pointer_cast<internal_t>(node);
            if (node_int->parent_ != nullptr)
            {
                node_int = ensure_enough_keys(node_int);
                nodes_.write_node(node_int->id_, node_int.get());
            }

            node = nodes_.load_node(node_int->children_.front());
        }

        b_leaf_ptr leaf = std::dynamic_pointer_cast<leaf_t>(node);

        out = remove_leaf(leaf, out);

        // Delete leaf
        nodes_.delete_node(leaf->id_);

        return out;
    }

    bool empty()
    {
        if (!root_)
            return true;

        b_node_ptr root = load_root();
        bool res = root->size() == 0;
        return res;
    }

private:
    using b_node_ptr = typename detail::b_node<Key, Value>::b_node_ptr;
    using b_internal_ptr = typename detail::b_node<Key, Value>::b_internal_ptr;
    using b_leaf_ptr = typename detail::b_node<Key, Value>::b_leaf_ptr;

    boost::optional<storage::node_id> root_;
    storage::memory<detail::b_node<Key, Value> > nodes_;

    using leaf_t = detail::b_leaf<Key, Value>;
    using internal_t = detail::b_internal<Key, Value>;

    bool is_leaf(b_node_ptr x)
    {
        b_leaf_ptr leaf = std::dynamic_pointer_cast<leaf_t>(x);
        return leaf != nullptr;
    }

    template <typename OutIter>
    OutIter remove_leaf(b_leaf_ptr leaf, OutIter out)
    {
        b_internal_ptr parent = leaf->load_parent();

        if (parent)
        {
            assert(parent->keys_.size() >= t || !parent->parent_);

            // Remove link to leaf from parent
            auto it = std::find(parent->children_.begin(), parent->children_.end(), leaf->id_);
            std::size_t i = it - parent->children_.begin();
            parent->keys_.erase(parent->keys_.begin() + i);
            parent->children_.erase(parent->children_.begin() + i);

            // If parent became empty (that could only happen if it was root), make new root
            if (parent->size() == 0)
            {
                assert(!parent->parent_);

                root_ = parent->children_.front();

                b_node_ptr root = load_root();
                if (root->parent_)
                {
                    nodes_.delete_node(*root->parent_);
                    root->parent_ = boost::none;
                }
                nodes_.write_node(root->id_, root.get());
            }
            else
                nodes_.write_node(parent->id_, parent.get());
        }
        else
            root_ = boost::none;

        // Copy all elements from leaf to given output iterator
        for (auto x : leaf->values_)
        {
            *out = std::move(x);
            ++out;
        }

        return out;
    }

    b_internal_ptr ensure_enough_keys(b_internal_ptr node)
    {
        if (node->parent_)
        {
            if (node->keys_.size() == t - 1)
            {
                b_internal_ptr parent = node->load_parent();

                // Find parent link to it
                auto it = std::find(parent->children_.begin(), parent->children_.end(), node->id_);
                std::size_t i = it - parent->children_.begin();

                // Check brothers
                if (i + 1 <= parent->size())
                {
                    b_internal_ptr right_brother = std::dynamic_pointer_cast<internal_t>(nodes_.load_node(parent->children_[i + 1]));

                    if (right_brother->keys_.size() >= t)
                    {
                        // Move left child from right brother to the node
                        node->children_.push_back(std::move(right_brother->children_.front()));
                        right_brother->children_.erase(right_brother->children_.begin());
                        {
                            b_node_ptr child = nodes_.load_node(node->children_.back());
                            child->parent_ = node->id_;
                            nodes_.write_node(child->id_, child.get());
                        }

                        // Update keys
                        node->keys_.push_back(std::move(parent->keys_[i]));
                        parent->keys_[i] = right_brother->keys_.front();
                        right_brother->keys_.erase(right_brother->keys_.begin());

                        nodes_.write_node(right_brother->id_, right_brother.get());
                        nodes_.write_node(parent->id_, parent.get());
                    }
                    else
                    {
                        nodes_.write_node(parent->id_, parent.get());
                        node->merge_with_right_brother(i, right_brother, t, root_);

                        // Delete right brother
                        nodes_.delete_node(right_brother->id_);
                    }
                }
                else
                {
                    b_internal_ptr left_brother = std::dynamic_pointer_cast<internal_t>(nodes_.load_node(parent->children_[i - 1]));

                    if (left_brother->keys_.size() >= t)
                    {
                        // Move right child from left brother to the node
                        node->children_.insert(node->children_.begin(), std::move(left_brother->children_.back()));
                        left_brother->children_.pop_back();
                        {
                            b_node_ptr child = nodes_.load_node(node->children_.front());
                            child->parent_ = node->id_;
                            nodes_.write_node(child->id_, child.get());
                        }

                        // Update keys
                        node->keys_.insert(node->keys_.begin(), std::move(parent->keys_[i - 1]));
                        parent->keys_[i - 1] = left_brother->keys_.back();
                        left_brother->keys_.pop_back();

                        nodes_.write_node(left_brother->id_, left_brother.get());
                        nodes_.write_node(parent->id_, parent.get());
                    }
                    else
                    {
                        nodes_.write_node(parent->id_, parent.get());
                        left_brother->merge_with_right_brother(i - 1, node, t, root_);

                        // Delete right brother
                        nodes_.delete_node(node->id_);

                        node = left_brother;
                    }
                }
            }
        }
        return node;
    }

    b_node_ptr load_root()
    {
        if (!root_)
        {
            b_leaf_ptr root = std::make_shared<leaf_t>(nodes_);
            root_ = root->id_;
            return root;
        }

        return nodes_.load_node(*root_);
    }
};
}
