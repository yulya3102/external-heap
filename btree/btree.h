#pragma once

#include <vector>
#include <map>
#include <cassert>
#include <algorithm>
#include <queue>
#include <exception>

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
struct b_node_data
{
    storage::node_id id_;
    boost::optional<storage::node_id> parent_;
    std::size_t level_;

    b_node_data()
    {
        throw std::logic_error(
            "b_node_data(): this constructor should never be called"
        );
    }

    b_node_data(const storage::node_id & id,
                const boost::optional<storage::node_id> & parent,
                std::size_t level)
        : id_(id)
        , parent_(parent)
        , level_(level)
    {}

    virtual ~b_node_data() = default;
};

template <typename Key, typename Value>
struct b_node : std::enable_shared_from_this<b_node<Key, Value> >, virtual b_node_data<Key, Value>
{
    using b_node_ptr = std::shared_ptr<b_node>;
    using b_internal_ptr = std::shared_ptr<b_internal<Key, Value> >;
    using b_buffer_ptr = std::shared_ptr<b_buffer<Key, Value> >;
    using b_leaf_ptr = std::shared_ptr<b_leaf<Key, Value> >;
    using cache_t = storage::cache<b_node>;

    virtual std::size_t size() const = 0;
    virtual b_node * copy(const storage::memory<b_node> & storage) const = 0;
    virtual b_node_ptr new_brother() const = 0;

    cache_t & storage_;

    b_node(cache_t & storage, const storage::node_id & id, std::size_t level)
        : b_node_data<Key, Value>{id, boost::none, level}
        , storage_(storage)
    {}

    b_node(const b_node & other, cache_t & storage)
        : b_node_data<Key, Value>{other.id_, other.parent_, other.level_}
        , storage_(storage)
    {}

    virtual ~b_node() = default;

    b_buffer_ptr load_parent() const
    {
        if (this->parent_)
            return std::dynamic_pointer_cast<b_buffer<Key, Value> >(storage_[*this->parent_]);
        return nullptr;
    }

    // Split node: replace the node with its parent (or new internal node)
    // make new node (right brother) of the same type as the node
    // and make the node and its new brother children of the new internal node
    // Then return new parent of the node
    virtual b_buffer_ptr split_full(b_buffer_ptr parent, size_t t, boost::optional<storage::node_id> & tree_root) = 0;

    // Ensure node is not too big
    // If its size == 2 * t - 1, split node and return its new parent
    // else return boost::none
    boost::optional<b_buffer_ptr> ensure_not_too_big(size_t t, boost::optional<storage::node_id> & tree_root)
    {
        if (this->size() == 2 * t - 1)
            return split_full(this->load_parent(), t, tree_root);

        return boost::none;
    }

    // Make correct links from (maybe new) parent to the node and its new right brother
    // and update all parent links and root link if necessary
    void update_parent(b_internal_ptr parent, b_node_ptr new_brother, boost::optional<storage::node_id> & tree_root)
    {
        // Replace link from parent to old child with two links:
        // to old child and to new child
        // or just add two new links if parent is empty
        {
            auto it = std::find(parent->children_.begin(), parent->children_.end(), this->id_);
            // *it == this || it == end
            if (it != parent->children_.end())
                it = parent->children_.erase(it);
            // *it == element after this
            // insert before it
            it = parent->children_.insert(it, new_brother->id_);
            // *it == new_brother
            // insert before it
            it = parent->children_.insert(it, this->id_);
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
    // If it can be done without changing the higher-level tree structure, do it and return boost::none
    // else change tree structure and return root of the changed tree
    virtual boost::optional<b_buffer_ptr> add(Key && key, Value && value, size_t t, boost::optional<storage::node_id> & tree_root) = 0;

    // Remove left leaf from subtree and return values from it
    virtual std::vector<std::pair<Key, Value> > remove_left_leaf(std::size_t t, boost::optional<storage::node_id> & tree_root) = 0;
};

template <typename Key, typename Value>
struct b_leaf_data : virtual b_node_data<Key, Value>
{
    std::vector<std::pair<Key, Value>> values_;
};

template <typename Key, typename Value>
struct b_leaf : b_node<Key, Value>, b_leaf_data<Key, Value>
{
    using b_internal_ptr = typename b_node<Key, Value>::b_internal_ptr;
    using b_node_ptr = typename b_node<Key, Value>::b_node_ptr;
    using b_leaf_ptr = typename b_node<Key, Value>::b_leaf_ptr;
    using b_buffer_ptr = typename b_node<Key, Value>::b_buffer_ptr;
    using cache_t = typename b_node<Key, Value>::cache_t;

    b_leaf(cache_t & storage, const storage::node_id & id)
        : b_node_data<Key, Value>(id, boost::none, 0)
        , b_node<Key, Value>(storage, id, 0)
    {}

    b_leaf(const b_leaf & other, cache_t & storage)
        : b_node_data<Key, Value>(other)
        , b_node<Key, Value>(other, storage)
        , b_leaf_data<Key, Value>(other)
    {}

    virtual std::size_t size() const
    {
        return this->values_.size();
    }

    virtual b_leaf * copy(const storage::memory<b_node<Key, Value> > & storage) const
    {
        auto cache = new cache_t(const_cast<storage::memory<b_node<Key, Value> > & >(storage));
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
            parent = b_buffer<Key, Value>::new_node(this->storage_, this->level_ + 1);
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

    virtual boost::optional<b_buffer_ptr> add(Key && key, Value && value, size_t t, boost::optional<storage::node_id> & tree_root)
    {
        auto r = this->ensure_not_too_big(t, tree_root);
        if (r)
            return r;

        auto v = std::make_pair(std::move(key), std::move(value));
        auto it = std::lower_bound(this->values_.begin(), this->values_.end(), v);
        this->values_.insert(it, std::move(v));

        return boost::none;
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

        return this->values_;
    }

    virtual std::vector<std::pair<Key, Value> > remove_left_leaf(std::size_t t, boost::optional<storage::node_id> & tree_root)
    {
        if (this->parent_)
        {
            auto r = this->load_parent()->ensure_enough_keys(t, tree_root);
            if (r)
                return (*r)->remove_left_leaf(t, tree_root);
        }

        auto result = this->remove(t, tree_root);
        this->storage_.delete_node(this->id_);
        return result;
    }
};

template <typename Key, typename Value>
struct b_internal_data : virtual b_node_data<Key, Value>
{
    std::vector<Key> keys_;
    std::vector<storage::node_id> children_;
};

template <typename Key, typename Value>
struct b_internal : b_node<Key, Value>, virtual b_internal_data<Key, Value>
{
    using b_internal_ptr = typename b_node<Key, Value>::b_internal_ptr;
    using b_node_ptr = typename b_node<Key, Value>::b_node_ptr;
    using b_buffer_ptr = typename b_node<Key, Value>::b_buffer_ptr;
    using cache_t = typename b_node<Key, Value>::cache_t;

    b_internal(cache_t & storage, const storage::node_id & id, std::size_t level)
        : b_node<Key, Value>(storage, id, level)
    {}

    b_internal(const b_internal & other, cache_t & storage)
        : b_internal_data<Key, Value>(other)
        , b_node<Key, Value>(other, storage)
    {}

    virtual std::size_t size() const
    {
        return this->keys_.size();
    }

    virtual ~b_internal() = default;

    virtual b_buffer_ptr split_full(b_buffer_ptr parent, size_t t, boost::optional<storage::node_id> & tree_root)
    {
        assert(this->size() == 2 * t - 1);
        assert(!parent || parent->size() < 2 * t - 1);

        if (!parent)
            parent = b_buffer<Key, Value>::new_node(this->storage_, this->level_ + 1);
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

    virtual boost::optional<b_buffer_ptr> add(Key && key, Value && value, size_t t, boost::optional<storage::node_id> & tree_root)
    {
        assert(std::is_sorted(this->keys_.begin(), this->keys_.end()));

        auto r = this->ensure_not_too_big(t, tree_root);
        if (r)
            return r;

        auto it = std::lower_bound(this->keys_.begin(), this->keys_.end(), key);
        std::size_t i = it - this->keys_.begin();
        b_node_ptr child = this->storage_[this->children_[i]];

        r = child->add(std::move(key), std::move(value), t, tree_root);
        if (!r)
            return boost::none;

        if ((*r)->level_ > this->level_)
            return r;

        return this->add(std::move(key), std::move(value), t, tree_root);
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

    std::pair<result_tag, b_buffer_ptr> get_right_brother(b_buffer_ptr parent, size_t t, boost::optional<storage::node_id> & tree_root)
    {
        assert(parent->pending_add_.empty());

        auto it = std::find(parent->children_.begin(), parent->children_.end(), this->id_);
        std::size_t i = it - parent->children_.begin();

        if (i + 1 > parent->size())
            return {result_tag::RESULT, nullptr};

        b_buffer_ptr right_brother = std::dynamic_pointer_cast<b_buffer<Key, Value> >(this->storage_[parent->children_[i + 1]]);
        if (right_brother->pending_add_.empty())
            return {result_tag::RESULT, right_brother};

        // right brother may want to split, so
        // parent should have < 2 * t - 1 keys
        assert(parent->size() < 2 * t - 1);
        auto r = right_brother->flush(t, tree_root);
        if (r)
            return {result_tag::CONTINUE_FROM, *r};

        return {result_tag::RESULT, right_brother};
    }

    // Ensure this node has enough keys (and children) to safely delete one
    // If it is, return boost::none
    // else steal one child from any brother or merge with brother
    // and return root of the changed tree
    boost::optional<b_buffer_ptr> ensure_enough_keys(std::size_t t, boost::optional<storage::node_id> & tree_root)
    {
        if (!this->parent_ || this->keys_.size() != t - 1)
            return boost::none;

        b_buffer_ptr parent = this->load_parent();

        // Find parent link to this node
        auto it = std::find(parent->children_.begin(), parent->children_.end(), this->id_);
        std::size_t i = it - parent->children_.begin();

        // Check brothers
        if (i + 1 <= parent->size())
        {
            std::pair<result_tag, b_buffer_ptr> changed_subtree = this->get_right_brother(parent, t, tree_root);
            if (changed_subtree.first == result_tag::CONTINUE_FROM)
                return changed_subtree.second;

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
                auto r = parent->ensure_enough_keys(t, tree_root);
                if (r)
                    return r;

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
                auto r = parent->ensure_enough_keys(t, tree_root);
                if (r)
                    return r;

                left_brother->merge_with_right_brother(
                            i - 1,
                            std::dynamic_pointer_cast<b_internal>(this->shared_from_this()),
                            t,
                            tree_root);

                // Delete right brother
                this->storage_.delete_node(this->id_);
            }
        }

        return parent;
    }

    virtual std::vector<std::pair<Key, Value>>
        remove_left_leaf(std::size_t t, boost::optional<storage::node_id> & tree_root)
    {
        return this->storage_[this->children_.front()]
                -> remove_left_leaf(t, tree_root);
    }
};

template <typename Key, typename Value>
struct b_buffer_data : virtual b_internal_data<Key, Value>
{
    std::queue<std::pair<Key, Value> > pending_add_;
};

template <typename Key, typename Value>
struct b_buffer : b_internal<Key, Value>, b_buffer_data<Key, Value>
{
    using b_buffer_ptr = typename b_node<Key, Value>::b_buffer_ptr;
    using b_internal_ptr = typename b_node<Key, Value>::b_internal_ptr;
    using b_node_ptr = typename b_node<Key, Value>::b_node_ptr;
    using cache_t = typename b_node<Key, Value>::cache_t;

    b_buffer(cache_t & storage, const storage::node_id & id, std::size_t level)
        : b_node_data<Key, Value>{id, boost::none, level}
        , b_internal<Key, Value>(storage, id, level)
    {}

    b_buffer(const b_buffer & other, cache_t & storage)
        : b_node_data<Key, Value>(other)
        , b_internal_data<Key, Value>(other)
        , b_internal<Key, Value>(other, storage)
        , b_buffer_data<Key, Value>(other)
    {}

    virtual b_buffer * copy(const storage::memory<b_node<Key, Value> > & storage) const
    {
        auto cache = new cache_t(const_cast<storage::memory<b_node<Key, Value> > & >(storage));
        return new b_buffer(*this, *cache);
    }

    static b_buffer_ptr new_node(cache_t & cache, std::size_t level)
    {
        return std::dynamic_pointer_cast<b_buffer>(
                    cache.new_node(
                        [&cache, level] (storage::node_id id)
                        { return new b_buffer(cache, id, level); }
                    ));
    }

    virtual b_node_ptr new_brother() const
    {
        return new_node(this->storage_, this->level_);
    }

    // Try to add all elements from pending list to the tree
    // If it can be done without changing the higher-level tree structure, do it and return boost::none
    // else change tree structure and return root of the changed tree
    boost::optional<b_buffer_ptr> flush(size_t t, boost::optional<storage::node_id> & tree_root)
    {
        while (!this->pending_add_.empty())
        {
            auto x = std::move(this->pending_add_.front());
            this->pending_add_.pop();
            boost::optional<b_buffer_ptr> r = b_internal<Key, Value>::add(std::move(x.first), std::move(x.second), t, tree_root);
            if (r)
            {
                (*r)->pending_add_.push(x);
                return r;
            }
        }

        return boost::none;
    }

    virtual b_buffer_ptr split_full(b_buffer_ptr parent, size_t t, boost::optional<storage::node_id> & tree_root)
    {
        parent = std::dynamic_pointer_cast<b_buffer>(b_internal<Key, Value>::split_full(parent, t, tree_root));

        while (!this->pending_add_.empty())
        {
            parent->pending_add_.push(std::move(this->pending_add_.front()));
            this->pending_add_.pop();
        }

        return parent;
    }

    virtual boost::optional<b_buffer_ptr> add(Key && key, Value && value, size_t t, boost::optional<storage::node_id> & tree_root)
    {
        if (this->pending_add_.size() == t)
        {
            boost::optional<b_buffer_ptr> r = this->flush(t, tree_root);
            if (r)
                return r;
        }

        this->pending_add_.push(std::make_pair(std::move(key), std::move(value)));
        return boost::none;
    }

    virtual std::vector<std::pair<Key, Value>>
        remove_left_leaf(std::size_t t, boost::optional<storage::node_id> & tree_root)
    {
        if (this->pending_add_.empty())
            return b_internal<Key, Value>::remove_left_leaf(t, tree_root);

        boost::optional<b_buffer_ptr> r = this->flush(t, tree_root);
        if (r)
            return (*r)->remove_left_leaf(t, tree_root);
        return b_internal<Key, Value>::remove_left_leaf(t, tree_root);
    }
};
}

namespace bptree
{
template <typename Key, typename Value, int t>
struct b_tree
{
    b_tree(storage::memory<detail::b_node<Key, Value>> & storage,
           boost::optional<storage::node_id> root = boost::none)
        : nodes_(storage)
        , root_(root)
    {}

    boost::optional<storage::node_id> root_id() const
    {
        return root_;
    }

    b_tree(const b_tree & other) = delete;

    void flush_cache()
    {
        nodes_.flush();
    }

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

    storage::cache<detail::b_node<Key, Value> > nodes_;
    boost::optional<storage::node_id> root_;

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
