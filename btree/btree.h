#pragma once

#include "btree_data.h"
#include "serialize.h"

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

template <typename Key, typename Value, typename Serialized>
struct b_internal;

template <typename Key, typename Value, typename Serialized>
struct b_buffer;

template <typename Key, typename Value, typename Serialized>
struct b_leaf;

template <typename Key, typename Value, typename Serialized>
struct b_node;

template <typename Key, typename Value, typename Serialized>
std::shared_ptr<b_node<Key, Value, Serialized>> node_constructor(const b_node_data<Key, Value> & data, storage::cache<b_node_data<Key, Value>, Serialized> & cache);

template <typename Key, typename Value, typename Serialized>
struct b_node
{
    using b_node_ptr = std::shared_ptr<b_node>;
    using b_internal_ptr = std::shared_ptr<b_internal<Key, Value, Serialized>>;
    using b_buffer_ptr = std::shared_ptr<b_buffer<Key, Value, Serialized>>;
    using b_leaf_ptr = std::shared_ptr<b_leaf<Key, Value, Serialized>>;
    using cache_t = storage::cache<b_node_data<Key, Value>, Serialized>;

    virtual std::size_t size() const = 0;
    virtual b_node * copy(cache_t & cache) const = 0;
    virtual storage::node_id new_brother() const = 0;

    storage::node_id id_;
    cache_t & storage_;

    b_node(cache_t & storage, const storage::node_id & id, std::size_t level)
        : id_(id)
        , storage_(storage)
    {}

    b_node(const b_node & other, cache_t & storage)
        : id_(other.id_)
        , storage_(storage)
    {}

    b_node(const b_node_data<Key, Value> & data, cache_t & cache)
        : id_(data.id_)
        , storage_(cache)
    {}

    virtual ~b_node() = default;

    virtual b_node_data<Key, Value> & cached_this() const
    {
        return *storage_[this->id_];
    }

    std::shared_ptr<b_buffer_data<Key, Value>> buffer(const storage::node_id & id) const
    {
        return std::dynamic_pointer_cast<b_buffer_data<Key, Value>>(storage_[id]);
    }

    b_buffer<Key, Value, Serialized> buffer_node(const storage::node_id & id) const
    {
        return b_buffer<Key, Value, Serialized>(*this->buffer(id), this->storage_);
    }

    std::shared_ptr<b_leaf_data<Key, Value>> leaf(const storage::node_id & id) const
    {
        return std::dynamic_pointer_cast<b_leaf_data<Key, Value>>(storage_[id]);
    }

    std::shared_ptr<b_buffer_data<Key, Value>> parent() const
    {
        if (cached_this().parent_)
            return buffer(*cached_this().parent_);
        return nullptr;
    }

    b_buffer<Key, Value, Serialized> parent_node() const
    {
        assert(static_cast<bool>(*cached_this().parent_));
        return buffer_node(*cached_this().parent_);
    }

    // Return index of the node in parent's 'children' array
    std::size_t child_index() const
    {
        auto it = std::find(parent()->children_.begin(), parent()->children_.end(), this->id_);
        return it - parent()->children_.begin();
    }

    // Split node: replace the node with its parent (or new internal node)
    // make new node (right brother) of the same type as the node
    // and make the node and its new brother children of the new internal node
    // If it can be done without changing the higher-level tree structure, do it and return { RESULT, new parent of the node }
    // else return { CONTINUE_FROM, node that should be split first }
    virtual std::pair<result_tag, storage::node_id> split_full(size_t t, boost::optional<storage::node_id> & tree_root) = 0;

    // Ensure node is not too big
    // If its size == 2 * t - 1, split node or someone above it and return parent of split node
    // else return boost::none
    boost::optional<storage::node_id> ensure_not_too_big(size_t t, boost::optional<storage::node_id> & tree_root)
    {
        if (this->size() == 2 * t - 1)
        {
            auto r = split_full(t, tree_root);
            return r.second;
        }

        return boost::none;
    }

    // Make correct links from (maybe new) parent to the node and its new right brother
    // and update all parent links and root link if necessary
    void update_parent(storage::node_id new_brother, boost::optional<storage::node_id> & tree_root)
    {
        // Replace link from parent to old child with two links:
        // to old child and to new child
        // or just add two new links if parent is empty
        {
            auto it = std::find(parent()->children_.begin(), parent()->children_.end(), this->id_);
            // *it == this || it == end
            if (it != parent()->children_.end())
                it = parent()->children_.erase(it);
            // *it == element after this
            // insert before it
            it = parent()->children_.insert(it, new_brother);
            // *it == new_brother
            // insert before it
            it = parent()->children_.insert(it, this->id_);
            // *it == this
        }

        // Update root link if necessary
        if (!cached_this().parent_)
        {
            tree_root = parent()->id_;
            parent()->parent_ = boost::none;
        }

        // Update old child and new child parent links
        cached_this().parent_ = parent()->id_;
        this->storage_[new_brother]->parent_ = parent()->id_;
    }

    // Try to add pair(key, value) to the tree.
    // If it can be done without changing the higher-level tree structure, do it and return boost::none
    // else change tree structure and return root of the changed tree
    virtual boost::optional<storage::node_id> add(Key && key, Value && value, size_t t, boost::optional<storage::node_id> & tree_root) = 0;

    // Remove left leaf from subtree and return values from it
    virtual std::vector<std::pair<Key, Value> > remove_left_leaf(std::size_t t, boost::optional<storage::node_id> & tree_root) = 0;
};

template <typename Key, typename Value, typename Serialized>
struct b_leaf : b_node<Key, Value, Serialized>
{
    using cache_t = typename b_node<Key, Value, Serialized>::cache_t;

    b_leaf(cache_t & storage, const storage::node_id & id)
        : b_node<Key, Value, Serialized>(storage, id, 0)
    {}

    b_leaf(const b_leaf & other, cache_t & storage)
        : b_node<Key, Value, Serialized>(other, storage)
    {}

    b_leaf(const b_leaf_data<Key, Value> & data, cache_t & cache)
        : b_node<Key, Value, Serialized>(static_cast<const b_node_data<Key, Value> &>(data), cache)
    {}

    virtual b_leaf_data<Key, Value> & cached_this() const
    {
        return *std::dynamic_pointer_cast<b_leaf_data<Key, Value>>(this->storage_[this->id_]);
    }

    virtual std::size_t size() const
    {
        return cached_this().values_.size();
    }

    virtual b_leaf * copy(cache_t & cache) const
    {
        std::shared_ptr<b_leaf_data<Key, Value>> x(this->leaf(this->id_)->copy_data());
        return new b_leaf(std::move(*x), cache);
    }

    virtual storage::node_id new_brother() const
    {
        return this->storage_.new_node([] (storage::node_id id) { return new b_leaf_data<Key, Value>(id); })->id_;
    }

    virtual ~b_leaf() = default;

    virtual std::pair<result_tag, storage::node_id> split_full(size_t t, boost::optional<storage::node_id> & tree_root)
    {
        assert(this->size() == 2 * t - 1);

        if (this->parent())
        {
            auto r = this->parent_node().ensure_not_too_big(t, tree_root);
            if (r)
                return { result_tag::CONTINUE_FROM, *r };
        }

        if (!this->parent())
        {
            cached_this().parent_ = b_buffer<Key, Value, Serialized>::new_node(this->storage_, cached_this().level_ + 1)->id_;
            tree_root = cached_this().parent_;
        }
        storage::node_id brother = this->new_brother();

        auto split_by_it = cached_this().values_.begin();
        for (size_t i = 0; i < t - 1; ++i)
            ++split_by_it;

        {
            // Insert new key after one pointing to x
            auto this_it = std::find(this->parent()->children_.begin(), this->parent()->children_.end(), this->id_);
            size_t this_i = this_it - this->parent()->children_.begin();
            auto this_key_it = this->parent()->keys_.begin() + this_i;
            this->parent()->keys_.insert(this_key_it, split_by_it->first);
        }

        for (auto it = split_by_it; it != cached_this().values_.end(); ++it)
            this->leaf(brother)->values_.push_back(std::move(*it));
        cached_this().values_.erase(split_by_it, cached_this().values_.end());

        // Make correct links from parent to the node and its new brother
        // and update all parent links
        this->update_parent(brother, tree_root);

        assert(static_cast<bool>(cached_this().parent_));
        return { result_tag::RESULT, *(cached_this().parent_) };
    }

    virtual boost::optional<storage::node_id> add(Key && key, Value && value, size_t t, boost::optional<storage::node_id> & tree_root)
    {
        auto r = this->ensure_not_too_big(t, tree_root);
        if (r)
            return r;

        auto v = std::make_pair(std::move(key), std::move(value));
        auto it = std::lower_bound(cached_this().values_.begin(), cached_this().values_.end(), v);
        cached_this().values_.insert(it, std::move(v));

        return boost::none;
    }

    // Remove this leaf from parent tree and return values from it
    std::vector<std::pair<Key, Value> > remove(std::size_t t, boost::optional<storage::node_id> & tree_root)
    {
        if (this->parent())
        {
            assert(this->parent()->keys_.size() >= t || !this->parent()->parent_);

            // Remove link to leaf from parent
            auto it = std::find(this->parent()->children_.begin(), this->parent()->children_.end(), this->id_);
            std::size_t i = it - this->parent()->children_.begin();
            this->parent()->keys_.erase(this->parent()->keys_.begin() + i);
            this->parent()->children_.erase(this->parent()->children_.begin() + i);

            // If parent became empty (that could only happen if it was root), make new root
            if (this->parent_node().size() == 0)
            {
                assert(!this->parent()->parent_);

                tree_root = this->parent()->children_.front();
                this->storage_.delete_node(this->parent()->id_);

                this->storage_[*tree_root]->parent_ = boost::none;
            }
        }
        else
            tree_root = boost::none;

        return cached_this().values_;
    }

    virtual std::vector<std::pair<Key, Value> > remove_left_leaf(std::size_t t, boost::optional<storage::node_id> & tree_root)
    {
        if (cached_this().parent_)
        {
            auto r = this->parent_node().ensure_enough_keys(t, tree_root);
            if (r)
                return this->buffer_node(*r).remove_left_leaf(t, tree_root);
        }

        auto result = this->remove(t, tree_root);
        this->storage_.delete_node(this->id_);
        return result;
    }
};

template <typename Key, typename Value, typename Serialized>
struct b_internal : b_node<Key, Value, Serialized>
{
    using cache_t = typename b_node<Key, Value, Serialized>::cache_t;

    b_internal(cache_t & storage, const storage::node_id & id, std::size_t level)
        : b_node<Key, Value, Serialized>(storage, id, level)
    {}

    b_internal(const b_internal & other, cache_t & storage)
        : b_node<Key, Value, Serialized>(other, storage)
    {}

    b_internal(const b_internal_data<Key, Value> & data, cache_t & cache)
        : b_node<Key, Value, Serialized>(static_cast<const b_node_data<Key, Value> &>(data), cache)
    {}

    virtual b_internal_data<Key, Value> & cached_this() const
    {
        return *std::dynamic_pointer_cast<b_internal_data<Key, Value>>(this->storage_[this->id_]);
    }

    virtual std::size_t size() const
    {
        return cached_this().keys_.size();
    }

    virtual ~b_internal() = default;

    virtual std::pair<result_tag, storage::node_id> split_full(size_t t, boost::optional<storage::node_id> & tree_root)
    {
        assert(this->size() == 2 * t - 1);

        if (this->parent())
        {
            auto r = this->parent_node().ensure_not_too_big(t, tree_root);
            if (r)
                return { result_tag::CONTINUE_FROM, *r };
        }

        if (!this->parent())
        {
            cached_this().parent_ = b_buffer<Key, Value, Serialized>::new_node(this->storage_, cached_this().level_ + 1)->id_;
            tree_root = cached_this().parent_;
        }
        storage::node_id brother = this->new_brother();

        auto split_keys = cached_this().keys_.begin() + (t - 1);
        auto split_children = cached_this().children_.begin() + t;
        auto it = std::find(this->parent()->children_.begin(), this->parent()->children_.end(), this->id_);
        auto i = it - this->parent()->children_.begin();
        auto i_key = this->parent()->keys_.begin() + i;

        this->parent()->keys_.insert(i_key, std::move(*split_keys));

        for (auto it_keys = split_keys + 1; it_keys != cached_this().keys_.end(); ++it_keys)
            this->buffer(brother)->keys_.push_back(std::move(*it_keys));

        for (auto it_children = split_children; it_children != cached_this().children_.end(); ++it_children)
        {
            this->buffer(brother)->children_.push_back(std::move(*it_children));
            storage::node_id child = this->buffer(brother)->children_.back();
            this->storage_[child]->parent_ = brother;
        }

        cached_this().keys_.erase(split_keys, cached_this().keys_.end());
        cached_this().children_.erase(split_children, cached_this().children_.end());

        // Make correct links from parent to the node and its new brother
        // and update all parent links
        this->update_parent(brother, tree_root);

        assert(static_cast<bool>(cached_this().parent_));
        return { result_tag::RESULT, *(cached_this().parent_) };
    }

    virtual boost::optional<storage::node_id> add(Key && key, Value && value, size_t t, boost::optional<storage::node_id> & tree_root)
    {
        assert(std::is_sorted(cached_this().keys_.begin(), cached_this().keys_.end()));

        auto r = this->ensure_not_too_big(t, tree_root);
        if (r)
            return r;

        auto it = std::lower_bound(cached_this().keys_.begin(), cached_this().keys_.end(), key);
        std::size_t i = it - cached_this().keys_.begin();
        storage::node_id child = cached_this().children_[i];

        r = detail::node_constructor(*this->storage_[child], this->storage_)
                ->add(std::move(key), std::move(value), t, tree_root);
        if (!r)
            return boost::none;

        if (this->buffer(*r)->level_ > cached_this().level_)
            return *r;

        return this->add(std::move(key), std::move(value), t, tree_root);
    }

    void merge_with_right_brother(std::size_t i, storage::node_id right_brother, std::size_t t, boost::optional<storage::node_id> & tree_root)
    {
        assert(cached_this().parent_ == this->storage_[right_brother]->parent_);

        assert(this->parent()->children_[i] == this->id_);
        assert(!this->parent()->parent_ || this->parent_node().size() > t - 1);

        // Move children from right brother to the node
        for (auto child_it = this->buffer(right_brother)->children_.begin();
             child_it != this->buffer(right_brother)->children_.end();
             ++child_it)
        {
            this->storage_[*child_it]->parent_ = this->id_;
            cached_this().children_.push_back(std::move(*child_it));
        }

        // Move key from parent to the node
        cached_this().keys_.push_back(std::move(this->parent()->keys_[i]));
        this->parent()->keys_.erase(this->parent()->keys_.begin() + i);

        // Move keys from right brother to the node
        for (auto key_it = this->buffer(right_brother)->keys_.begin();
             key_it != this->buffer(right_brother)->keys_.end();
             ++key_it)
            cached_this().keys_.push_back(std::move(*key_it));

        // Remove link to right brother from parent
        this->parent()->children_.erase(this->parent()->children_.begin() + i + 1);

        // If parent became empty (could happen only if it is root and had only one key)
        // then make the node new root
        if (this->parent_node().size() == 0)
        {
            tree_root = this->id_;
            this->storage_.delete_node(this->parent()->id_);
            cached_this().parent_ = boost::none;
        }
    }

    std::pair<result_tag, boost::optional<storage::node_id>> get_right_brother(size_t t, boost::optional<storage::node_id> & tree_root)
    {
        assert(this->parent()->pending_add_.empty());

        auto it = std::find(this->parent()->children_.begin(), this->parent()->children_.end(), this->id_);
        std::size_t i = it - this->parent()->children_.begin();

        if (i + 1 > this->parent_node().size())
            return {result_tag::RESULT, boost::none};

        storage::node_id right_brother = this->parent()->children_[i + 1];
        if (this->buffer(right_brother)->pending_add_.empty())
            return {result_tag::RESULT, right_brother};

        // right brother may want to split, so
        // parent should have < 2 * t - 1 keys
        assert(this->parent_node().size() < 2 * t - 1);
        auto r = this->buffer_node(right_brother).flush(t, tree_root);
        if (r)
            return {result_tag::CONTINUE_FROM, *r };

        return {result_tag::RESULT, right_brother};
    }

    // Ensure this node has enough keys (and children) to safely delete one
    // If it is, return boost::none
    // else steal one child from any brother or merge with brother
    // and return root of the changed tree
    boost::optional<storage::node_id> ensure_enough_keys(std::size_t t, boost::optional<storage::node_id> & tree_root)
    {
        if (!cached_this().parent_ || cached_this().keys_.size() != t - 1)
            return boost::none;

        // Find parent link to this node
        auto it = std::find(this->parent()->children_.begin(), this->parent()->children_.end(), this->id_);
        std::size_t i = it - this->parent()->children_.begin();

        // Check brothers
        if (i + 1 <= this->parent_node().size())
        {
            std::pair<result_tag, boost::optional<storage::node_id>> changed_subtree = this->get_right_brother(t, tree_root);
            if (changed_subtree.first == result_tag::CONTINUE_FROM)
                return changed_subtree.second;

            storage::node_id right_brother = *changed_subtree.second;

            if (this->buffer(right_brother)->keys_.size() >= t)
            {
                // Move left child from right brother to the node
                cached_this().children_.push_back(std::move(this->buffer(right_brother)->children_.front()));
                this->buffer(right_brother)->children_.erase(this->buffer(right_brother)->children_.begin());
                    this->storage_[cached_this().children_.back()]->parent_ = this->id_;

                // Update keys
                cached_this().keys_.push_back(std::move(this->parent()->keys_[i]));
                this->parent()->keys_[i] = this->buffer(right_brother)->keys_.front();
                this->buffer(right_brother)->keys_.erase(this->buffer(right_brother)->keys_.begin());
            }
            else
            {
                auto r = this->parent_node().ensure_enough_keys(t, tree_root);
                if (r)
                    return r;

                this->merge_with_right_brother(i, right_brother, t, tree_root);

                // Delete right brother
                this->storage_.delete_node(right_brother);
            }
        }
        else
        {
            storage::node_id left_brother = this->parent()->children_[i - 1];

            if (this->buffer(left_brother)->keys_.size() >= t)
            {
                // Move right child from left brother to the node
                cached_this().children_.insert(cached_this().children_.begin(), std::move(this->buffer(left_brother)->children_.back()));
                this->buffer(left_brother)->children_.pop_back();
                    this->storage_[cached_this().children_.front()]->parent_ = this->id_;

                // Update keys
                cached_this().keys_.insert(cached_this().keys_.begin(), std::move(this->parent()->keys_[i - 1]));
                this->parent()->keys_[i - 1] = this->buffer(left_brother)->keys_.back();
                this->buffer(left_brother)->keys_.pop_back();
            }
            else
            {
                auto r = this->parent_node().ensure_enough_keys(t, tree_root);
                if (r)
                    return r;

                this->buffer_node(left_brother).merge_with_right_brother(i - 1, this->id_, t, tree_root);

                // Delete right brother
                this->storage_.delete_node(this->id_);
            }
        }

        if (!cached_this().parent_)
            return this->id_;

        return cached_this().parent_;
    }

    virtual std::vector<std::pair<Key, Value>>
        remove_left_leaf(std::size_t t, boost::optional<storage::node_id> & tree_root)
    {
        return node_constructor(*this->storage_[cached_this().children_.front()], this->storage_)
                -> remove_left_leaf(t, tree_root);
    }
};

template <typename Key, typename Value, typename Serialized>
struct b_buffer : b_internal<Key, Value, Serialized>
{
    using b_buffer_ptr = typename b_node<Key, Value, Serialized>::b_buffer_ptr;
    using cache_t = typename b_node<Key, Value, Serialized>::cache_t;

    b_buffer(cache_t & storage, const storage::node_id & id, std::size_t level)
        : b_internal<Key, Value, Serialized>(storage, id, level)
    {}

    b_buffer(const b_buffer & other, cache_t & storage)
        : b_internal<Key, Value, Serialized>(other, storage)
    {}

    b_buffer(const b_buffer_data<Key, Value> & data, cache_t & cache)
        : b_internal<Key, Value, Serialized>(static_cast<const b_internal_data<Key, Value> &>(data), cache)
    {}

    b_buffer_data<Key, Value> & cached_this() const
    {
        return *std::dynamic_pointer_cast<b_buffer_data<Key, Value>>(this->storage_[this->id_]);
    }

    virtual b_buffer * copy(cache_t & cache) const
    {
        std::shared_ptr<b_buffer_data<Key, Value>> x(this->buffer(this->id_)->copy_data());
        return new b_buffer(std::move(*x), cache);
    }

    static std::shared_ptr<b_buffer_data<Key, Value>> new_node(cache_t & cache, std::size_t level)
    {
        return std::dynamic_pointer_cast<b_buffer_data<Key, Value>>(
                    cache.new_node(
                        [level] (storage::node_id id)
                        { return new b_buffer_data<Key, Value>(id, level); }
                    ));
    }

    virtual storage::node_id new_brother() const
    {
        return new_node(this->storage_, cached_this().level_)->id_;
    }

    // Try to add all elements from pending list to the tree
    // If it can be done without changing the higher-level tree structure, do it and return boost::none
    // else change tree structure and return root of the changed tree
    boost::optional<storage::node_id> flush(size_t t, boost::optional<storage::node_id> & tree_root)
    {
        while (!cached_this().pending_add_.empty())
        {
            auto x = std::move(cached_this().pending_add_.front());
            cached_this().pending_add_.pop();
            boost::optional<storage::node_id> r = b_internal<Key, Value, Serialized>::add(std::move(x.first), std::move(x.second), t, tree_root);
            if (r)
            {
                this->buffer(*r)->pending_add_.push(x);
                return r;
            }
        }

        return boost::none;
    }

    virtual std::pair<result_tag, storage::node_id> split_full(size_t t, boost::optional<storage::node_id> & tree_root)
    {
        auto r = b_internal<Key, Value, Serialized>::split_full(t, tree_root);

        if (r.first == result_tag::CONTINUE_FROM)
            return r;

        // r.first == result_tag::RESULT
        // it means there were no higher-level splits

        assert(cached_this().parent_ == r.second);

        std::queue<std::pair<Key, Value>> keep_pending;
        while (!cached_this().pending_add_.empty())
        {
            auto x = cached_this().pending_add_.front();
            cached_this().pending_add_.pop();

            std::size_t i = this->child_index();
            Key left = std::numeric_limits<Key>::min(),
                right = std::numeric_limits<Key>::max();
            if (i > 0)
                left = this->parent()->keys_[i - 1];
            if (i < this->parent()->keys_.size())
                right = this->parent()->keys_[i];

            std::pair<Key, Key> range(left, right);
            if (x.first < range.first)
            {
                // push x to left brother
                this->buffer(this->parent()->children_[i - 1])
                        ->pending_add_.push(std::move(x));
            }
            else if (x.first < range.second)
                keep_pending.push(std::move(x));
            else
            {
                // push x to right brother
                this->buffer(this->parent()->children_[i + 1])
                        ->pending_add_.push(std::move(x));
            }
        }
        cached_this().pending_add_.swap(keep_pending);

        assert(static_cast<bool>(cached_this().parent_));
        return { result_tag::RESULT, *(cached_this().parent_) };
    }

    virtual boost::optional<storage::node_id> add(Key && key, Value && value, size_t t, boost::optional<storage::node_id> & tree_root)
    {
        if (cached_this().pending_add_.size() >= t)
        {
            boost::optional<storage::node_id> r = this->flush(t, tree_root);
            if (r)
                return r;
        }

        cached_this().pending_add_.push(std::make_pair(std::move(key), std::move(value)));
        return boost::none;
    }

    virtual std::vector<std::pair<Key, Value>>
        remove_left_leaf(std::size_t t, boost::optional<storage::node_id> & tree_root)
    {
        if (cached_this().pending_add_.empty())
            return b_internal<Key, Value, Serialized>::remove_left_leaf(t, tree_root);

        boost::optional<storage::node_id> r = this->flush(t, tree_root);
        if (r)
            return this->buffer_node(*r).remove_left_leaf(t, tree_root);
        return b_internal<Key, Value, Serialized>::remove_left_leaf(t, tree_root);
    }
};

template <typename Key, typename Value, typename Serialized>
std::shared_ptr<b_node<Key, Value, Serialized>> node_constructor(const b_node_data<Key, Value> & data, storage::cache<b_node_data<Key, Value>, Serialized> & cache)
{
    if (const b_leaf_data<Key, Value> * leaf_data
        = dynamic_cast<const b_leaf_data<Key, Value> *>(&data))
        return std::shared_ptr<b_node<Key, Value, Serialized>>(new b_leaf<Key, Value, Serialized>(*leaf_data, cache));

    if (const b_buffer_data<Key, Value> * buffer_data
        = dynamic_cast<const b_buffer_data<Key, Value> *>(&data))
        return std::shared_ptr<b_node<Key, Value, Serialized>>(new b_buffer<Key, Value, Serialized>(*buffer_data, cache));

    throw std::logic_error("Unknown node type");
}
}

namespace bptree
{
template <typename Key, typename Value, typename Serialized = std::string>
struct b_tree
{
    using data = detail::b_node_data<Key, Value>;
    using serializer_t = std::function<Serialized *(data *)>;
    using deserializer_t = std::function<data *(Serialized *)>;

    b_tree(storage::basic_storage<Serialized> & storage,
           std::size_t t,
           boost::optional<storage::node_id> root = boost::none,
           serializer_t serializer = bptree::serialize,
           deserializer_t deserializer = bptree::deserialize)
        : nodes_(storage,
                 deserializer,
                 serializer)
        , t_(t)
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

        boost::optional<storage::node_id> r(root->id_);
        do
        {
            // TODO: fix double move
            r = detail::node_constructor(*nodes_[*r], nodes_)
                    ->add(std::move(key), std::move(value), t_, root_);
        }
        while (r);
    }

    template <typename OutIter>
    OutIter remove_left_leaf(OutIter out)
    {
        b_node_ptr node = load_root();

        for (auto x : detail::node_constructor(*node, nodes_)->remove_left_leaf(t_, root_))
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
        bool res = detail::node_constructor(*root, nodes_)->size() == 0;
        return res;
    }

private:
    using b_node_ptr = typename std::shared_ptr<detail::b_node_data<Key, Value>>;
    using b_internal_ptr = typename detail::b_node<Key, Value, Serialized>::b_internal_ptr;
    using b_leaf_ptr = typename detail::b_node<Key, Value, Serialized>::b_leaf_ptr;
    using b_buffer_ptr = typename detail::b_node<Key, Value, Serialized>::b_buffer_ptr;

    using cache_t = storage::cache<detail::b_node_data<Key, Value>, Serialized>;
    cache_t nodes_;
    std::size_t t_;
    boost::optional<storage::node_id> root_;

    using leaf_t = detail::b_leaf<Key, Value, Serialized>;
    using internal_t = detail::b_internal<Key, Value, Serialized>;

    bool is_leaf(b_node_ptr x)
    {
        b_leaf_ptr leaf = std::dynamic_pointer_cast<leaf_t>(x);
        return leaf != nullptr;
    }

    b_node_ptr load_root()
    {
        if (!root_)
        {
            b_node_ptr root = nodes_.new_node([] (storage::node_id id) { return new detail::b_leaf_data<Key, Value>(id); });
            root_ = root->id_;
            return root;
        }

        return nodes_[*root_];
    }
};
}
