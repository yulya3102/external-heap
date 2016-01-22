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

private:
    struct b_node
    {
        using b_node_ptr = b_node *;

        virtual std::size_t size() const = 0;

        b_node_ptr parent_;
    };

    using b_node_ptr = typename b_node::b_node_ptr;

    struct b_leaf : b_node
    {
        std::map<Key, Value> values_;

        virtual std::size_t size() const
        {
            return values_.size();
        }
    };

    struct b_internal : b_node
    {
        std::vector<Key> keys_;
        std::vector<b_node_ptr> children_;

        virtual std::size_t size() const
        {
            return keys_.size();
        }
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
        assert(x->parent_->size() < 2 * t - 1);

        // Split node: replace x with new internal node s,
        // make new node y of the same type as x
        // and make x and y children of the new node
        b_internal * s = new b_internal();
        b_node_ptr y;

        if (is_leaf(x))
        {
            b_leaf * x_leaf = dynamic_cast<b_leaf *>(x);
            b_leaf * y_leaf = new b_leaf();

            auto split_by_it = x_leaf->values_.begin();
            for (size_t i = 0; i < t - 1; ++i)
                ++split_by_it;

            s->keys_.push_back(split_by_it->first);
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

        // Update link from x->parent_ to s
        if (!x->parent_) // parent link is null, x is the root node
            root_ = s;
        else             // find child pointing to x and replace it
        {
            b_internal * parent = dynamic_cast<b_internal *>(x->parent_);
            *std::find(parent->children_.begin(), parent->children_.end(), x) = s;
        }

        // Update s, x and y parent links
        s->parent_ = x->parent_;
        x->parent_ = s;
        y->parent_ = s;

        return s;
    }

};
}
