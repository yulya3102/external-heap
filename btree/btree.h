#pragma once

#include <vector>
#include <map>
#include <cassert>
#include <algorithm>

namespace detail
{
template <typename Key>
struct b_internal;

template <typename Key>
struct b_node
{
    using b_node_ptr = b_node *;

    virtual std::size_t size() const = 0;

    b_internal<Key> * parent_;

    virtual ~b_node() = default;

    // Split node: replace the node with its parent (or new internal node)
    // make new node (right brother) of the same type as the node
    // and make the node and its new brother children of the new internal node
    // Then return new parent of the node
    virtual b_internal<Key> * split_full(size_t t, b_node_ptr & tree_root) = 0;

    // Make correct links from (maybe new) parent to the node and its new right brother
    // and update all parent links and root link if necessary
    void update_parent(b_internal<Key> * parent, b_node_ptr new_brother, b_node_ptr & tree_root)
    {
        // Replace link from parent to old child with two links:
        // to old child and to new child
        // or just add two new links if parent is empty
        {
            auto it = std::find(parent->children_.begin(), parent->children_.end(), this);
            // *it == this || it == end
            if (it != parent->children_.end())
                it = parent->children_.erase(it);
            // *it == element after this
            // insert before it
            it = parent->children_.insert(it, new_brother);
            // *it == new_brother
            // insert before it
            it = parent->children_.insert(it, this);
            // *it == this
        }

        // Update root link if necessary
        if (!this->parent_)
        {
            tree_root = parent;
            parent->parent_ = nullptr;
        }

        // Update old child and new child parent links
        this->parent_ = parent;
        new_brother->parent_ = parent;
    }
};

template <typename Key>
using b_node_ptr = typename b_node<Key>::b_node_ptr;

template <typename Key, typename Value>
struct b_leaf : b_node<Key>
{
    std::vector<std::pair<Key, Value> > values_;

    virtual std::size_t size() const
    {
        return values_.size();
    }

    virtual ~b_leaf() = default;

    b_internal<Key> * split_full(size_t t, b_node<Key> * & tree_root)
    {
        assert(this->size() == 2 * t - 1);
        assert(!this->parent_ || this->parent_->size() < 2 * t - 1);

        b_internal<Key> * parent = this->parent_ ? this->parent_ : new b_internal<Key>();
        b_leaf<Key, Value> * brother = new b_leaf<Key, Value>();

        auto split_by_it = this->values_.begin();
        for (size_t i = 0; i < t - 1; ++i)
            ++split_by_it;

        {
            // Insert new key after one pointing to x
            auto this_it = std::find(parent->children_.begin(), parent->children_.end(), this);
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

        return this->parent_;
    }
};

template<typename Key>
struct b_internal : b_node<Key>
{
    std::vector<Key> keys_;
    std::vector<b_node_ptr<Key> > children_;

    virtual std::size_t size() const
    {
        return keys_.size();
    }

    virtual ~b_internal() = default;

    b_internal<Key> * split_full(size_t t, b_node<Key> * & tree_root)
    {
        assert(this->size() == 2 * t - 1);
        assert(!this->parent_ || this->parent_->size() < 2 * t - 1);

        b_internal<Key> * parent = this->parent_ ? this->parent_ : new b_internal<Key>();
        b_internal<Key> * brother = new b_internal<Key>();

        auto split_keys = this->keys_.begin() + (t - 1);
        auto split_children = this->children_.begin() + t;

        parent->keys_.push_back(std::move(*split_keys));

        for (auto it_keys = split_keys + 1; it_keys != this->keys_.end(); ++it_keys)
            brother->keys_.push_back(std::move(*it_keys));

        for (auto it_children = split_children; it_children != this->children_.end(); ++it_children)
        {
            brother->children_.push_back(std::move(*it_children));
            brother->children_.back()->parent_ = brother;
        }

        this->keys_.erase(split_keys, this->keys_.end());
        this->children_.erase(split_children, this->children_.end());

        // Make correct links from parent to the node and its new brother
        // and update all parent links
        this->update_parent(parent, brother, tree_root);

        return this->parent_;
    }
};
}

namespace bptree
{
template <typename Key, typename Value, int t>
struct b_tree
{
    b_tree()
        : root_(new leaf_t())
    { }

    void add(Key && key, Value && value)
    {
        b_node_ptr node = get_root();

        if (node->size() == 2 * t - 1)
            node = node->split_full(t, root_);

        while (!is_leaf(node))
        {
            internal_t * int_node = dynamic_cast<internal_t *>(node);

            auto it = std::lower_bound(int_node->keys_.begin(), int_node->keys_.end(), key);
            std::size_t i = it - int_node->keys_.begin();
            node = int_node->children_[i];

            if (node->size() == 2 * t - 1)
                node = node->split_full(t, root_);
        }

        leaf_t * leaf = dynamic_cast<leaf_t *>(node);

        auto v = std::make_pair(std::move(key), std::move(value));
        auto it = std::lower_bound(leaf->values_.begin(), leaf->values_.end(), v);
        leaf->values_.insert(it, std::move(v));
    }

    template <typename OutIter>
    OutIter remove_left_leaf(OutIter out)
    {
        b_node_ptr node = get_root();

        while (!is_leaf(node))
        {
            internal_t * node_int = dynamic_cast<internal_t *>(node);
            if (node_int->parent_ != nullptr)
                node_int = ensure_enough_keys(node_int);

            node = node_int->children_.front();
        }

        leaf_t * leaf = dynamic_cast<leaf_t *>(node);

        return remove_leaf(leaf, out);
    }

    bool empty() const
    {
        return root_ == nullptr || root_->size() == 0;
    }

private:
    using b_node_ptr = detail::b_node_ptr<Key>;
    b_node_ptr root_;

    using leaf_t = detail::b_leaf<Key, Value>;
    using internal_t = detail::b_internal<Key>;

    bool is_leaf(b_node_ptr x)
    {
        leaf_t * leaf = dynamic_cast<leaf_t *>(x);
        return leaf != nullptr;
    }

    template <typename OutIter>
    OutIter remove_leaf(leaf_t * leaf, OutIter out)
    {
        internal_t * parent = leaf->parent_;

        if (parent)
        {
            assert(parent->keys_.size() >= t || parent->parent_ == nullptr);

            // Remove link to leaf from parent
            auto it = std::find(parent->children_.begin(), parent->children_.end(), leaf);
            std::size_t i = it - parent->children_.begin();
            parent->keys_.erase(parent->keys_.begin() + i);
            parent->children_.erase(parent->children_.begin() + i);

            // If parent became empty (that could only happen if it was root), make new root
            if (parent->size() == 0)
            {
                assert(parent->parent_ == nullptr);

                root_ = parent->children_.front();
                delete root_->parent_;
                root_->parent_ = nullptr;
            }
        }
        else
            root_ = nullptr;

        // Copy all elements from leaf to given output iterator
        for (auto x : leaf->values_)
        {
            *out = std::move(x);
            ++out;
        }

        // Delete leaf
        delete leaf;

        return out;
    }

    internal_t * merge_brothers(internal_t * left_brother, std::size_t i, internal_t * right_brother)
    {
        assert(left_brother->parent_ == right_brother->parent_);
        internal_t * parent = left_brother->parent_;

        assert(parent->children_[i] == left_brother);
        assert(parent->parent_ == nullptr || parent->size() > t - 1);

        // Move children from right brother to the node
        for (auto child_it = right_brother->children_.begin();
             child_it != right_brother->children_.end();
             ++child_it)
        {
            (*child_it)->parent_ = left_brother;
            left_brother->children_.push_back(std::move(*child_it));
        }

        // Move key from parent to the node
        left_brother->keys_.push_back(std::move(parent->keys_[i]));
        parent->keys_.erase(parent->keys_.begin() + i);

        // Move keys from right brother to the node
        for (auto key_it = right_brother->keys_.begin();
             key_it != right_brother->keys_.end();
             ++key_it)
            left_brother->keys_.push_back(std::move(*key_it));

        // Remove link to right brother from parent
        parent->children_.erase(parent->children_.begin() + i + 1);

        // Delete right brother
        delete right_brother;

        // If parent became empty (could happen only if it is root and had only one key)
        // then make the node new root
        if (parent->size() == 0)
        {
            root_ = left_brother;
            delete parent;
            left_brother->parent_ = nullptr;
        }

        return left_brother;
    }

    internal_t * ensure_enough_keys(internal_t * node)
    {
        if (node->parent_ != nullptr)
        {
            if (node->keys_.size() == t - 1)
            {
                internal_t * parent = node->parent_;

                // Find parent link to it
                auto it = std::find(parent->children_.begin(), parent->children_.end(), node);
                std::size_t i = it - parent->children_.begin();

                // Check brothers
                if (i + 1 <= parent->size())
                {
                    internal_t * right_brother = dynamic_cast<internal_t *>(parent->children_[i + 1]);

                    if (right_brother->keys_.size() >= t)
                    {
                        // Move left child from right brother to the node
                        node->children_.push_back(std::move(right_brother->children_.front()));
                        right_brother->children_.erase(right_brother->children_.begin());
                        node->children_.back()->parent_ = node;

                        // Update keys
                        node->keys_.push_back(std::move(parent->keys_[i]));
                        parent->keys_[i] = right_brother->keys_.front();
                        right_brother->keys_.erase(right_brother->keys_.begin());
                    }
                    else
                        node = merge_brothers(node, i, right_brother);
                }
                else
                {
                    internal_t * left_brother = dynamic_cast<internal_t *>(parent->children_[i - 1]);

                    if (left_brother->keys_.size() >= t)
                    {
                        // Move right child from left brother to the node
                        node->children_.insert(node->children_.begin(), std::move(left_brother->children_.back()));
                        left_brother->children_.pop_back();
                        node->children_.front()->parent_ = node;

                        // Update keys
                        node->keys_.insert(node->keys_.begin(), std::move(parent->keys_[i - 1]));
                        parent->keys_[i - 1] = left_brother->keys_.back();
                        left_brother->keys_.pop_back();
                    }
                    else
                        node = merge_brothers(left_brother, i - 1, node);
                }

            }
        }
        return node;
    }

    b_node_ptr get_root()
    {
        if (!root_)
            root_ = new leaf_t();

        return root_;
    }
};
}
