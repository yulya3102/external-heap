#pragma once

#include <map>

namespace bptree
{
struct b_node
{
    using b_node_ptr = b_node *;
};

template <typename Key, typename Value, int t>
struct b_leaf : b_node
{
    std::map<Key, Value> values_;
};

template <typename Key, typename Value, int t>
struct b_internal_node: b_node
{
    std::map<Key, b_node_ptr> children_;
};
}
