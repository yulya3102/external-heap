#pragma once

#include <vector>
#include <map>
#include <cassert>
#include <algorithm>
#include <queue>

#include <boost/optional.hpp>

#include <storage/cache.h>

namespace detail
{
enum struct result_tag
{
    RESULT,
    CONTINUE_FROM
};

template <typename Key, typename Value>
struct b_internal;

template <typename Key, typename Value>
struct b_buffer;

template <typename Key, typename Value>
struct b_leaf;

template <typename Key, typename Value>
struct b_node : std::enable_shared_from_this<b_node<Key, Value> >
{
    using b_node_ptr = std::shared_ptr<b_node>;
    using b_internal_ptr = std::shared_ptr<b_internal<Key, Value> >;
    using b_buffer_ptr = std::shared_ptr<b_buffer<Key, Value> >;
    using b_leaf_ptr = std::shared_ptr<b_leaf<Key, Value> >;

    virtual std::size_t size() const = 0;
    virtual b_node * copy(const storage::memory<b_node> & storage) const = 0;
    virtual b_node_ptr new_brother() const = 0;

    storage::cache<b_node> & storage_;
    storage::node_id id_;
    boost::optional<storage::node_id> parent_;

    b_node(storage::cache<b_node> & storage, const storage::node_id & id)
        : storage_(storage)
        , id_(id)
        , parent_(boost::none)
    {}

    b_node(const b_node & other, storage::cache<b_node> & storage)
        : storage_(storage)
        , id_(other.id_)
        , parent_(other.parent_)
    {}

    virtual ~b_node() = default;

    b_buffer_ptr load_parent() const
    {
        if (parent_)
            return std::dynamic_pointer_cast<b_buffer<Key, Value> >(storage_[*parent_]);
        return nullptr;
    }

    // Split node: replace the node with its parent (or new internal node)
    // make new node (right brother) of the same type as the node
    // and make the node and its new brother children of the new internal node
    // Then return new parent of the node
    virtual b_buffer_ptr split_full(b_buffer_ptr parent, size_t t, boost::optional<storage::node_id> & tree_root) = 0;

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

    // Try to add pair(key, value) to the tree.
    // If it can be done without changing the tree structure, do it and return boost::none
    // else change tree structure and return root of the changed tree
    virtual boost::optional<b_node_ptr> add(Key && key, Value && value, size_t t, boost::optional<storage::node_id> & tree_root) = 0;

    // Remove left leaf from subtree and return values from it
    virtual std::vector<std::pair<Key, Value> > remove_left_leaf(std::size_t t, boost::optional<storage::node_id> & tree_root) = 0;
};

template <typename Key, typename Value>
struct b_leaf : b_node<Key, Value>
{
    using b_internal_ptr = typename b_node<Key, Value>::b_internal_ptr;
    using b_node_ptr = typename b_node<Key, Value>::b_node_ptr;
    using b_leaf_ptr = typename b_node<Key, Value>::b_leaf_ptr;
    using b_buffer_ptr = typename b_node<Key, Value>::b_buffer_ptr;

    std::vector<std::pair<Key, Value> > values_;

    b_leaf(storage::cache<b_node<Key, Value> > & storage, const storage::node_id & id)
        : b_node<Key, Value>(storage, id)
    {}

    b_leaf(const b_leaf & other, storage::cache<b_node<Key, Value> > & storage)
        : b_node<Key, Value>(other, storage)
        , values_(other.values_)
    {}

    virtual std::size_t size() const
    {
        return values_.size();
    }

    virtual b_leaf * copy(const storage::memory<b_node<Key, Value> > & storage) const
    {
        auto cache = new storage::cache<b_node<Key, Value>>(const_cast<storage::memory<b_node<Key, Value> > & >(storage));
        return new b_leaf(*this, *cache);
    }

    virtual b_node_ptr new_brother() const
    {
        return this->storage_.new_node([this] (storage::node_id id) { return new b_leaf(this->storage_, id); });
    }

    virtual ~b_leaf() = default;

    virtual b_buffer_ptr split_full(b_buffer_ptr parent, size_t t, boost::optional<storage::node_id> & tree_root)
    {
        assert(this->size() == 2 * t - 1);
        assert(!parent || parent->size() < 2 * t - 1);

        if (!parent)
            parent = b_buffer<Key, Value>::new_node(this->storage_);
        b_leaf_ptr brother = std::dynamic_pointer_cast<b_leaf>(this->new_brother());

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

        return parent;
    }

    virtual boost::optional<b_node_ptr> add(Key && key, Value && value, size_t t, boost::optional<storage::node_id> & tree_root)
    {
        if (size() == 2 * t - 1)
        {
            b_buffer_ptr parent = this->load_parent();
            return std::dynamic_pointer_cast<b_node<Key, Value>>(this->split_full(parent, t, tree_root));
        }
        else
        {
            auto v = std::make_pair(std::move(key), std::move(value));
            auto it = std::lower_bound(this->values_.begin(), this->values_.end(), v);
            this->values_.insert(it, std::move(v));

            return boost::none;
        }
    }

    // Remove this leaf from parent tree and return values from it
    std::vector<std::pair<Key, Value> > remove(std::size_t t, boost::optional<storage::node_id> & tree_root)
    {
        b_buffer_ptr parent = this->load_parent();

        if (parent)
        {
            assert(parent->keys_.size() >= t || !parent->parent_);

            // Remove link to leaf from parent
            auto it = std::find(parent->children_.begin(), parent->children_.end(), this->id_);
            std::size_t i = it - parent->children_.begin();
            parent->keys_.erase(parent->keys_.begin() + i);
            parent->children_.erase(parent->children_.begin() + i);

            // If parent became empty (that could only happen if it was root), make new root
            if (parent->size() == 0)
            {
                assert(!parent->parent_);

                tree_root = parent->children_.front();
                this->storage_.delete_node(parent->id_);

                b_node_ptr root = this->storage_[*tree_root];
                root->parent_ = boost::none;
            }
        }
        else
            tree_root = boost::none;

        return values_;
    }

    virtual std::vector<std::pair<Key, Value> > remove_left_leaf(std::size_t t, boost::optional<storage::node_id> & tree_root)
    {
        auto result = this->remove(t, tree_root);
        this->storage_.delete_node(this->id_);
        return result;
    }
};

template <typename Key, typename Value>
struct b_internal : b_node<Key, Value>
{
    using b_internal_ptr = typename b_node<Key, Value>::b_internal_ptr;
    using b_node_ptr = typename b_node<Key, Value>::b_node_ptr;
    using b_buffer_ptr = typename b_node<Key, Value>::b_buffer_ptr;

    std::vector<Key> keys_;
    std::vector<storage::node_id> children_;

    b_internal(storage::cache<b_node<Key, Value> > & storage, const storage::node_id & id)
        : b_node<Key, Value>(storage, id)
    {}

    b_internal(const b_internal & other, storage::cache<b_node<Key, Value> > & storage)
        : b_node<Key, Value>(other, storage)
        , keys_(other.keys_)
        , children_(other.children_)
    {}

    virtual std::size_t size() const
    {
        return keys_.size();
    }

    virtual ~b_internal() = default;

    virtual b_buffer_ptr split_full(b_buffer_ptr parent, size_t t, boost::optional<storage::node_id> & tree_root)
    {
        assert(this->size() == 2 * t - 1);
        assert(!parent || parent->size() < 2 * t - 1);

        if (!parent)
            parent = b_buffer<Key, Value>::new_node(this->storage_);
        b_internal_ptr brother = std::dynamic_pointer_cast<b_internal>(this->new_brother());

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
            b_node_ptr child = this->storage_[brother->children_.back()];
            child->parent_ = brother->id_;
        }

        this->keys_.erase(split_keys, this->keys_.end());
        this->children_.erase(split_children, this->children_.end());

        // Make correct links from parent to the node and its new brother
        // and update all parent links
        this->update_parent(parent, brother, tree_root);

        return parent;
    }

    virtual boost::optional<b_node_ptr> add(Key && key, Value && value, size_t t, boost::optional<storage::node_id> & tree_root)
    {
        assert(std::is_sorted(keys_.begin(), keys_.end()));

        if (size() == 2 * t - 1)
        {
            b_buffer_ptr parent = this->load_parent();
            return std::dynamic_pointer_cast<b_node<Key, Value>>(this->split_full(parent, t, tree_root));
        }
        else
        {
            auto it = std::lower_bound(keys_.begin(), keys_.end(), key);
            std::size_t i = it - keys_.begin();
            b_node_ptr child = this->storage_[children_[i]];

            return child->add(std::move(key), std::move(value), t, tree_root);
        }
    }

    void merge_with_right_brother(std::size_t i, b_internal_ptr right_brother, std::size_t t, boost::optional<storage::node_id> & tree_root)
    {
        assert(this->parent_ == right_brother->parent_);
        b_buffer_ptr parent = this->load_parent();

        assert(parent->children_[i] == this->id_);
        assert(!parent->parent_ || parent->size() > t - 1);

        // Move children from right brother to the node
        for (auto child_it = right_brother->children_.begin();
             child_it != right_brother->children_.end();
             ++child_it)
        {
            b_node_ptr child = this->storage_[*child_it];
            child->parent_ = this->id_;
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
    }

    std::pair<result_tag, b_internal_ptr> get_right_brother(b_buffer_ptr parent, size_t t, boost::optional<storage::node_id> & tree_root)
    {
        assert(parent->pending_add_.empty());

        auto it = std::find(parent->children_.begin(), parent->children_.end(), this->id_);
        std::size_t i = it - parent->children_.begin();

        if (i + 1 > parent->size())
            return {result_tag::RESULT, nullptr};

        b_buffer_ptr right_brother = std::dynamic_pointer_cast<b_buffer<Key, Value> >(this->storage_[parent->children_[i + 1]]);
        if (right_brother->pending_add_.empty())
            return {result_tag::RESULT, right_brother};

        b_internal_ptr changed_subtree = std::dynamic_pointer_cast<b_internal<Key, Value>>(right_brother->flush(t, tree_root));
        if (changed_subtree->id_ != right_brother->id_)
            // Tree structure has been changed, looking for brother does not make sense anymore
            return {result_tag::CONTINUE_FROM, changed_subtree};

        return {result_tag::RESULT, changed_subtree};
    }

    // Ensure this node has enough keys (and children) to safely delete one
    // If it's not, steal one child from any brother or merge with brother
    // Return node with desired amount of keys
    std::pair<result_tag, b_internal_ptr> ensure_enough_keys(std::size_t t, boost::optional<storage::node_id> & tree_root)
    {
        if (!this->parent_ || this->keys_.size() != t - 1)
            return {result_tag::RESULT, std::dynamic_pointer_cast<b_internal>(this->shared_from_this())};

        b_buffer_ptr parent = this->load_parent();

        // Find parent link to this node
        auto it = std::find(parent->children_.begin(), parent->children_.end(), this->id_);
        std::size_t i = it - parent->children_.begin();

        // Check brothers
        if (i + 1 <= parent->size())
        {
            std::pair<result_tag, b_internal_ptr> changed_subtree = this->get_right_brother(parent, t, tree_root);
            if (changed_subtree.first == result_tag::CONTINUE_FROM)
                return changed_subtree;

            b_internal_ptr right_brother = changed_subtree.second;

            if (right_brother->keys_.size() >= t)
            {
                // Move left child from right brother to the node
                this->children_.push_back(std::move(right_brother->children_.front()));
                right_brother->children_.erase(right_brother->children_.begin());
                {
                    b_node_ptr child = this->storage_[this->children_.back()];
                    child->parent_ = this->id_;
                }

                // Update keys
                this->keys_.push_back(std::move(parent->keys_[i]));
                parent->keys_[i] = right_brother->keys_.front();
                right_brother->keys_.erase(right_brother->keys_.begin());
            }
            else
            {
                this->merge_with_right_brother(i, right_brother, t, tree_root);

                // Delete right brother
                this->storage_.delete_node(right_brother->id_);
            }
        }
        else
        {
            b_internal_ptr left_brother = std::dynamic_pointer_cast<b_internal>(this->storage_[parent->children_[i - 1]]);

            if (left_brother->keys_.size() >= t)
            {
                // Move right child from left brother to the node
                this->children_.insert(this->children_.begin(), std::move(left_brother->children_.back()));
                left_brother->children_.pop_back();
                {
                    b_node_ptr child = this->storage_[this->children_.front()];
                    child->parent_ = this->id_;
                }

                // Update keys
                this->keys_.insert(this->keys_.begin(), std::move(parent->keys_[i - 1]));
                parent->keys_[i - 1] = left_brother->keys_.back();
                left_brother->keys_.pop_back();
            }
            else
            {
                left_brother->merge_with_right_brother(
                            i - 1,
                            std::dynamic_pointer_cast<b_internal>(this->shared_from_this()),
                            t,
                            tree_root);

                // Delete right brother
                this->storage_.delete_node(this->id_);

                return {result_tag::RESULT, left_brother};
            }
        }

        return {result_tag::RESULT, std::dynamic_pointer_cast<b_internal>(this->shared_from_this())};
    }

    virtual std::vector<std::pair<Key, Value>>
        remove_left_leaf(std::size_t t, boost::optional<storage::node_id> & tree_root)
    {
        b_internal_ptr node = std::dynamic_pointer_cast<b_internal>(this->shared_from_this());

        if (this->parent_)
        {
            auto changed_subtree = this->ensure_enough_keys(t, tree_root);
            if (changed_subtree.first == result_tag::CONTINUE_FROM)
                return changed_subtree.second->remove_left_leaf(t, tree_root);

            node = changed_subtree.second;
        }

        return this->storage_[node->children_.front()]
            -> remove_left_leaf(t, tree_root);
    }
};

template <typename Key, typename Value>
struct b_buffer : b_internal<Key, Value>
{
    using b_buffer_ptr = typename b_node<Key, Value>::b_buffer_ptr;
    using b_internal_ptr = typename b_node<Key, Value>::b_internal_ptr;
    using b_node_ptr = typename b_node<Key, Value>::b_node_ptr;

    std::queue<std::pair<Key, Value> > pending_add_;

    b_buffer(storage::cache<b_node<Key, Value> > & storage, const storage::node_id & id)
        : b_internal<Key, Value>(storage, id)
    {}

    b_buffer(const b_buffer & other, storage::cache<b_node<Key, Value> > & storage)
        : b_internal<Key, Value>(other)
        , pending_add_(other.pending_add_)
    {}

    virtual b_buffer * copy(const storage::memory<b_node<Key, Value> > & storage) const
    {
        auto cache = new storage::cache<b_node<Key, Value>>(const_cast<storage::memory<b_node<Key, Value> > & >(storage));
        return new b_buffer(*this, *cache);
    }

    static b_buffer_ptr new_node(storage::cache<b_node<Key, Value>> & cache)
    {
        return std::dynamic_pointer_cast<b_buffer>(cache.new_node([&cache] (storage::node_id id) { return new b_buffer(cache, id); }));
    }

    virtual b_node_ptr new_brother() const
    {
        return new_node(this->storage_);
    }

    // Flush the subtree
    // Return root of flushed subtree
    b_node_ptr flush(size_t t, boost::optional<storage::node_id> & tree_root)
    {
        while (!pending_add_.empty())
        {
            auto x = std::move(pending_add_.front());
            pending_add_.pop();
            boost::optional<b_node_ptr> r = b_internal<Key, Value>::add(std::move(x.first), std::move(x.second), t, tree_root);
            while (r)
            {
                // TODO: fix double std::move()
                r = (*r)->add(std::move(x.first), std::move(x.second), t, tree_root);
            }
        }

        return this->shared_from_this();
    }

    virtual b_buffer_ptr split_full(b_buffer_ptr parent, size_t t, boost::optional<storage::node_id> & tree_root)
    {
        parent = std::dynamic_pointer_cast<b_buffer>(b_internal<Key, Value>::split_full(parent, t, tree_root));

        while (!this->pending_add_.empty())
        {
            parent->pending_add_.push(std::move(this->pending_add_.front()));
            pending_add_.pop();
        }

        return parent;
    }

    virtual boost::optional<b_node_ptr> add(Key && key, Value && value, size_t t, boost::optional<storage::node_id> & tree_root)
    {
        if (this->pending_add_.size() == t)
        {
            b_node_ptr next_add = this->flush(t, tree_root);
            return next_add->add(std::move(key), std::move(value), t, tree_root);
        }

        pending_add_.push(std::make_pair(std::move(key), std::move(value)));
        return boost::none;
    }

    virtual std::vector<std::pair<Key, Value>>
        remove_left_leaf(std::size_t t, boost::optional<storage::node_id> & tree_root)
    {
        if (this->pending_add_.empty())
            return b_internal<Key, Value>::remove_left_leaf(t, tree_root);

        b_node_ptr next = this->flush(t, tree_root);
        return next->remove_left_leaf(t, tree_root);
    }
};
}

namespace bptree
{
template <typename Key, typename Value, int t>
struct b_tree
{
    b_tree()
        : nodes_(storage_)
    {}

    void add(Key key, Value value)
    {
        b_node_ptr root = load_root();

        boost::optional<b_node_ptr> r(root);
        do
        {
            // TODO: fix double move
            r = (*r)->add(std::move(key), std::move(value), t, root_);
        }
        while (r);
    }

    template <typename OutIter>
    OutIter remove_left_leaf(OutIter out)
    {
        b_node_ptr node = load_root();

        for (auto x : node->remove_left_leaf(t, root_))
        {
            *out = std::move(x);
            ++out;
        }

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
    using b_buffer_ptr = typename detail::b_node<Key, Value>::b_buffer_ptr;

    boost::optional<storage::node_id> root_;
    storage::memory<detail::b_node<Key, Value> > storage_;
    storage::cache<detail::b_node<Key, Value> > nodes_;

    using leaf_t = detail::b_leaf<Key, Value>;
    using internal_t = detail::b_internal<Key, Value>;

    bool is_leaf(b_node_ptr x)
    {
        b_leaf_ptr leaf = std::dynamic_pointer_cast<leaf_t>(x);
        return leaf != nullptr;
    }

    b_node_ptr load_root()
    {
        if (!root_)
        {
            b_node_ptr root = nodes_.new_node([this] (storage::node_id id) { return new leaf_t(this->nodes_, id); });
            root_ = root->id_;
            return root;
        }

        return nodes_[*root_];
    }
};
}
