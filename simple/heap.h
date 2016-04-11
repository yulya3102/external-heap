#pragma once

#include <utils/undefined.h>
#include <storage/directory.h>

#include <utility>
#include <algorithm>
#include <vector>
#include <sstream>

namespace simple
{
template <typename Key, typename Value>
struct heap
{
    heap()
        : nodes("simple_storage")
    {
        save_nodes_order({});
    }

    void add(Key k, Value v)
    {
        auto s = std::to_string(k) + " " + std::to_string(v);
        auto id = next_id();
        nodes.write_node(id, &s);
        auto nodes_order = load_nodes_order();
        nodes_order.push_back(id);
        std::push_heap(nodes_order.begin(), nodes_order.end(),
                       [this] (storage::node_id a, storage::node_id b)
        {
            return this->comparator(a, b);
        });
        save_nodes_order(nodes_order);
    }

    std::pair<Key, Value> remove_min()
    {
        auto nodes_order = load_nodes_order();
        auto id = nodes_order.front();
        auto node = nodes.load_node(id);
        std::pop_heap(nodes_order.begin(), nodes_order.end(),
                      [this] (storage::node_id a, storage::node_id b)
        {
            return this->comparator(a, b);
        });
        nodes_order.pop_back();
        nodes.delete_node(id);
        save_nodes_order(nodes_order);

        std::size_t x;
        Key key = std::stoull(*node, &x);
        Value value = std::stoull(node->substr(x));
        return {key, value};
    }

    bool empty() const
    {
        auto nodes_order = load_nodes_order();
        return nodes_order.empty();
    }

private:
    bool comparator(storage::node_id a, storage::node_id b)
    {
        auto a_node = nodes.load_node(a);
        auto b_node = nodes.load_node(b);

        Key a_key = std::stoull(*a_node);
        Key b_key = std::stoull(*b_node);

        return a_key > b_key;
    }

    storage::node_id next_id() const
    {
        static storage::node_id id = 1;
        return id++;
    }

    void save_nodes_order(const std::vector<storage::node_id> & nodes_order)
    {
        std::stringstream ss;
        ss << std::to_string(nodes_order.size()) << " ";
        for (auto id : nodes_order)
        {
            ss << std::to_string(id) << " ";
        }
        auto s = ss.str();
        nodes.write_node(0, &s);
    }

    std::vector<storage::node_id> load_nodes_order() const
    {
        auto node = nodes.load_node(0);
        std::vector<storage::node_id> result;

        std::size_t x;
        std::size_t size = std::stoul(*node, &x);
        for (std::size_t i = 0; i < size; i++)
        {
            std::size_t off = x;
            storage::node_id id = std::stoull(node->substr(x), &off);
            result.push_back(id);
            x += off;
        }

        return result;
    }

    storage::directory<std::string> nodes;
};
}
