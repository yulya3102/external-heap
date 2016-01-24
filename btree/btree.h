#pragma once

#include <vector>
#include <map>
#include <cassert>
#include <algorithm>

namespace bptree
{
template <typename Key, typename Value, int t>
struct b_tree
{
    b_tree()
        : root_(new b_leaf())
    { }

    void add(Key && key, Value && value)
    {
        b_node_ptr node = root_;

        if (node->size() == 2 * t - 1)
            node = split_full(node);

        while (!is_leaf(node))
        {
            b_internal * int_node = dynamic_cast<b_internal *>(node);

            auto it = std::lower_bound(int_node->keys_.begin(), int_node->keys_.end(), key);
            std::size_t i = it - int_node->keys_.begin();
            node = int_node->children_[i];

            if (node->size() == 2 * t - 1)
                node = split_full(node);
        }

        b_leaf * leaf = dynamic_cast<b_leaf *>(node);
        leaf->values_[key] = value;
    }

    template <typename OutIter>
    OutIter remove_left_leaf(OutIter out)
    {
        b_node_ptr node = root_;

        while (!is_leaf(node))
        {
            b_internal * node_int = dynamic_cast<b_internal *>(node);
            if (node_int->parent_ != nullptr)
                node_int = ensure_enough_keys(node_int);

            node = node_int->children_.front();
        }

        b_leaf * leaf = dynamic_cast<b_leaf *>(node);

        return remove_leaf(leaf, out);
    }

    bool empty() const
    {
        return root_ == nullptr || root_->size() == 0;
    }

private:
    struct b_node;

    using b_node_ptr = typename b_node::b_node_ptr;

    struct b_leaf : b_node
    {
        std::map<Key, Value> values_;

        virtual std::size_t size() const
        {
            return values_.size();
        }

        virtual ~b_leaf() = default;
    };

    struct b_internal : b_node
    {
        std::vector<Key> keys_;
        std::vector<b_node_ptr> children_;

        virtual std::size_t size() const
        {
            return keys_.size();
        }

        virtual ~b_internal() = default;
    };

    struct b_node
    {
        using b_node_ptr = b_node *;

        virtual std::size_t size() const = 0;

        b_internal * parent_;

        virtual ~b_node() = default;
    };

    b_node_ptr root_;

    bool is_leaf(b_node_ptr x)
    {
        b_leaf * leaf = dynamic_cast<b_leaf *>(x);
        return leaf != nullptr;
    }

    b_node_ptr split_full(b_node_ptr x)
    {
        assert(x->size() == 2 * t - 1);
        assert(!x->parent_ || x->parent_->size() < 2 * t - 1);

        // Split node: replace x with its parent s (or new internal node)
        // make new node y of the same type as x
        // and make x and y children of the new node
        b_internal * s = x->parent_ ? x->parent_ : new b_internal();
        b_node_ptr y;

        if (is_leaf(x))
        {
            b_leaf * x_leaf = dynamic_cast<b_leaf *>(x);
            b_leaf * y_leaf = new b_leaf();

            auto split_by_it = x_leaf->values_.begin();
            for (size_t i = 0; i < t - 1; ++i)
                ++split_by_it;

            {
                // Insert new key after one pointing to x
                auto x_it = std::find(s->children_.begin(), s->children_.end(), x);
                size_t x_i = x_it - s->children_.begin();
                auto x_key_it = s->keys_.begin() + x_i;
                s->keys_.insert(x_key_it, split_by_it->first);
            }

            for (auto it = split_by_it; it != x_leaf->values_.end(); ++it)
                y_leaf->values_[it->first] = std::move(it->second);
            x_leaf->values_.erase(split_by_it, x_leaf->values_.end());

            y = y_leaf;
        }
        else
        {
            b_internal * x_int = dynamic_cast<b_internal *>(x);
            b_internal * y_int = new b_internal();

            auto split_keys = x_int->keys_.begin() + (t - 1);
            auto split_children = x_int->children_.begin() + t;

            s->keys_.push_back(std::move(*split_keys));

            for (auto it_keys = split_keys + 1; it_keys != x_int->keys_.end(); ++it_keys)
                y_int->keys_.push_back(std::move(*it_keys));

            for (auto it_children = split_children; it_children != x_int->children_.end(); ++it_children)
            {
                y_int->children_.push_back(std::move(*it_children));
                y_int->children_.back()->parent_ = y_int;
            }

            x_int->keys_.erase(split_keys, x_int->keys_.end());
            x_int->children_.erase(split_children, x_int->children_.end());

            y = y_int;
        }

        // Replace link from s to x with two links: to x and y
        // or just add two new links if s is empty
        {
            auto it = std::find(s->children_.begin(), s->children_.end(), x);
            // *it == x || it == end
            if (it != s->children_.end())
                it = s->children_.erase(it);
            // *it == element after x
            // insert before it
            it = s->children_.insert(it, y);
            // *it == y
            // insert before it
            it = s->children_.insert(it, x);
            // *it == x
        }

        // Update root link if necessary
        if (!x->parent_)
        {
            root_ = s;
            s->parent_ = nullptr;
        }

        // Update s, x and y parent links
        x->parent_ = s;
        y->parent_ = s;

        return s;
    }

    template <typename OutIter>
    OutIter remove_leaf(b_leaf * leaf, OutIter out)
    {
        b_internal * parent = leaf->parent_;

        if (parent)
        {
            assert(parent->keys_.size() >= t || (parent->parent_ == nullptr && parent->keys_.size() >= 2));

            // Remove link to leaf from parent
            auto it = std::find(parent->children_.begin(), parent->children_.end(), leaf);
            std::size_t i = it - parent->children_.begin();
            parent->keys_.erase(parent->keys_.begin() + i);
            parent->children_.erase(parent->children_.begin() + i);
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

    b_internal * merge_brothers(b_internal * left_brother, std::size_t i, b_internal * right_brother)
    {
        assert(left_brother->parent_ == right_brother->parent_);
        assert(left_brother->parent_->children_[i] == left_brother);
        assert(left_brother->parent_->parent_ == nullptr || left_brother->parent_->size() > t - 1);

        // Move children from right brother to the node
        for (auto child_it = right_brother->children_.begin();
             child_it != right_brother->children_.end();
             ++child_it)
        {
            (*child_it)->parent_ = left_brother;
            left_brother->children_.push_back(std::move(*child_it));
        }

        // Move key from parent to the node
        left_brother->keys_.push_back(std::move(left_brother->parent_->keys_[i]));
        left_brother->parent_->keys_.erase(left_brother->parent_->keys_.begin() + i);

        // Move keys from right brother to the node
        for (auto key_it = right_brother->keys_.begin();
             key_it != right_brother->keys_.end();
             ++key_it)
            left_brother->keys_.push_back(std::move(*key_it));

        // Remove link to right brother from parent
        left_brother->parent_->children_.erase(left_brother->parent_->children_.begin() + i + 1);

        // Delete right brother
        delete right_brother;

        // If parent became empty (could happen only if it is root and had only one key)
        // then make the node new root
        if (left_brother->parent_->size() == 0)
        {
            root_ = left_brother;
            delete left_brother->parent_;
            left_brother->parent_ = nullptr;
        }

        return left_brother;
    }

    b_internal * ensure_enough_keys(b_internal * node)
    {
        if (node->parent_ != nullptr)
        {
            if (node->keys_.size() == t - 1)
            {
                // Find parent link to it
                auto it = std::find(node->children_.begin(), node->children_.end(), node);
                std::size_t i = it - node->children_.begin();

                // Check brothers
                if (i + 1 < node->parent_->size())
                {
                    b_internal * right_brother = dynamic_cast<b_internal *>(node->children_[i + 1]);

                    if (right_brother->keys_.size() >= t)
                    {
                        // Move left child from right brother to the node
                        node->children_.push_back(std::move(right_brother->children_.front()));
                        right_brother->children_.erase(right_brother->children_.begin());
                        node->children_.back()->parent_ = node;

                        // Update keys
                        node->keys_.push_back(std::move(node->parent_->keys_[i]));
                        node->parent_->keys_[i] = right_brother->keys_.front();
                        right_brother->keys_.erase(right_brother->keys_.begin());
                    }
                    else
                        node = merge_brothers(node, i, right_brother);
                }
                else
                {
                    b_internal * left_brother = dynamic_cast<b_internal *>(node->children_[i - 1]);

                    if (left_brother->keys_.size() >= t)
                    {
                        // Move right child from left brother to the node
                        node->children_.insert(node->children_.begin(), std::move(left_brother->children_.back()));
                        left_brother->children_.pop_back();
                        node->children_.front()->parent_ = node;

                        // Update keys
                        node->keys_.insert(node->keys_.begin(), std::move(node->parent_->keys_[i - 1]));
                        node->parent_->keys_[i - 1] = left_brother->keys_.back();
                        left_brother->keys_.pop_back();
                    }
                    else
                        node = merge_brothers(left_brother, i - 1, node);
                }

            }
        }
        return node;
    }
};
}
