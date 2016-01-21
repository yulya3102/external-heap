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
    };

    struct b_internal : b_node
    {
        std::vector<Key> keys_;
        std::vector<b_node_ptr> children_;
    };

    b_node_ptr root_;

    bool is_leaf(b_node_ptr x)
    {

    }

    b_node_ptr split_full(b_node_ptr x)
    {
        assert(x->size() == 2 * t - 1);


    }

};
}
