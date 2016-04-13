#pragma once

#include "basic_storage.h"

#include <memory>
#include <list>
#include <unordered_map>
#include <algorithm>
#include <functional>

namespace storage
{
template <typename Node, typename Stored>
struct cache
{
    using deserializer_t = std::function<Node *(Stored *)>;
    using serializer_t = std::function<Stored *(Node *)>;

    cache(basic_storage<Stored> & storage, deserializer_t deserializer, serializer_t serializer, std::size_t cache_limit = 3)
        : storage_(storage)
        , deserializer(deserializer)
        , serializer(serializer)
        , cache_limit(cache_limit)
    {}

    std::shared_ptr<Node> new_node(std::function<Node *(node_id)> construct)
    {
        node_id id = storage_.new_node();
        std::shared_ptr<Node> node(construct(id));
        cached_nodes.emplace(id, node);
        recently_used(id);
        return node;
    }

    std::shared_ptr<Node> operator[](const node_id & id)
    {
        auto it = cached_nodes.find(id);
        if (it != cached_nodes.end())
        {
            remove_from_lru(id);
            recently_used(id);
            return it->second;
        }

        return load_node(id);
    }

    void delete_node(const node_id & id)
    {
        storage_.delete_node(id);
        cached_nodes.erase(id);
        remove_from_lru(id);
    }

    void flush()
    {
        for (auto node : cached_nodes)
        {
            std::shared_ptr<Stored> serialized(serializer(node.second.get()));
            storage_.write_node(node.first, serialized.get());
        }
    }

    ~cache()
    {
        flush();
    }

private:
    std::shared_ptr<Node> load_node(const node_id & id)
    {
        std::shared_ptr<Stored> x = storage_.load_node(id);
        std::shared_ptr<Node> node(deserializer(x.get()));
        cached_nodes.emplace(id, node);
        recently_used(id);
        return node;
    }

    void write_node(const node_id & id)
    {
        std::shared_ptr<Node> node = cached_nodes[id];
        std::shared_ptr<Stored> serialized(serializer(node.get()));
        storage_.write_node(id, serialized.get());
    }

    void write_node(const node_id & id, Node * node)
    {
        storage_.write_node(id, node);
    }

    void remove_from_lru(const node_id & id)
    {
        auto lru_it = std::find(last_recently_used.begin(), last_recently_used.end(), id);
        last_recently_used.erase(lru_it);
    }

    void recently_used(const node_id & id)
    {
        if (last_recently_used.size() == cache_limit)
        {
            node_id flushed = last_recently_used.front();
            last_recently_used.pop_front();
            write_node(flushed);
            cached_nodes.erase(flushed);
        }
        last_recently_used.push_back(id);
    }

    basic_storage<Stored> & storage_;
    deserializer_t deserializer;
    serializer_t serializer;
    std::unordered_map<node_id, std::shared_ptr<Node>> cached_nodes;
    std::size_t cache_limit;
    std::list<node_id> last_recently_used;
};
}
