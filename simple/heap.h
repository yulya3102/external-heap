#pragma once

#include <utils/undefined.h>
#include <storage/directory.h>

#include <utility>
#include <algorithm>

namespace simple
{
template <typename Key, typename Value>
struct heap
{
    heap()
        : nodes("simple_storage")
    {}

    void add(Key k, Value v)
    {
        auto s = std::to_string(k) + " " + std::to_string(v);
        auto id = next_id();
        nodes.write_node(id, &s);
        nodes_order.push_back(id);
        std::make_heap(nodes_order.begin(), nodes_order.end(),
                       [this] (Key a, Key b)
        {
            auto a_node = nodes.load_node(a);
            auto b_node = nodes.load_node(b);

            Key a_key = std::stoull(*a_node);
            Key b_key = std::stoull(*b_node);

            return a_key < b_key;
        });
    }

    std::pair<Key, Value> remove_min()
    {
        auto id = nodes_order.front();
        auto node = nodes.load_node(id);
        nodes.delete_node(id);

        std::size_t x;
        Key key = std::stoull(*node, &x);
        Value value = std::stoull(node->substr(x));
        return {key, value};
    }

    bool empty()
    {
        return nodes_order.empty();
    }

private:
    storage::node_id next_id() const
    {
        static storage::node_id id = 0;
        return id++;
    }

    storage::directory<std::string> nodes;
    std::vector<Key> nodes_order;
};
}
