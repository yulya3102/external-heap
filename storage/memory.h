#pragma once

#include <unordered_map>

namespace storage
{
template <typename Node>
struct node_traits {};

template <typename Node>
struct memory
{
    using node_id = int;

    node_id new_node() const
    {
        return storage_.size();
    }

    Node load_node(const node_id & id) const
    {
        Node result = node_traits<Node>::deserialize(storage_.at(id));
        return std::move(result);
    }

    void delete_node(const node_id & id)
    {
        storage_.erase(id);
    }

    void write_node(const node_id & id, const Node & node)
    {
        storage_[id] = node_traits<Node>::serialize(node);
    }

private:
    std::unordered_map<node_id, typename node_traits<Node>::serialized_t> storage_;
};
}
