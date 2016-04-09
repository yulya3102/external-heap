#pragma once

#include "memory.h"

#include <memory>
#include <list>
#include <algorithm>
#include <functional>

namespace storage
{
template <typename Node>
struct cache
{
    cache(memory<Node> & storage)
        : storage_(storage)
    {}

    std::shared_ptr<Node> new_node(std::function<Node *(node_id)> construct)
    {
        node_id id = storage_.new_node();
        std::shared_ptr<Node> node(construct(id));
        cached_nodes.push_back({id, node});
        return node;
    }

    std::shared_ptr<Node> operator[](const node_id & id)
    {
        for (auto node : cached_nodes)
            if (node.first == id)
                return node.second;
        return load_node(id);
    }

    void delete_node(const node_id & id)
    {
        storage_.delete_node(id);
        auto it = std::find_if(cached_nodes.begin(), cached_nodes.end(),
                               [id] (const cached_node & x) { return id == x.first; });
        cached_nodes.erase(it);
    }

    void flush()
    {
        for (auto node : cached_nodes)
            storage_.write_node(node.first, node.second.get());
    }

    ~cache()
    {
        flush();
    }

private:
    std::shared_ptr<Node> load_node(const node_id & id)
    {
        cached_nodes.push_back({id, storage_.load_node(id)});
        return cached_nodes.back().second;
    }

    void write_node(const node_id & id, Node * node)
    {
        storage_.write_node(id, node);
    }

    memory<Node> & storage_;
    using cached_node = std::pair<node_id, std::shared_ptr<Node>>;
    std::list<cached_node> cached_nodes;
};
}
