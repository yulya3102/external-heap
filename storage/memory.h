#pragma once

#include <unordered_map>

namespace storage
{
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
        return storage_.at(id);
    }

    void delete_node(const node_id & id)
    {
        storage_.erase(id);
    }

    void write_node(const node_id & id, const Node & node)
    {
        storage_[id] = node;
    }

private:
    std::unordered_map<node_id, Node> storage_;
};
}
