#pragma once

#include <unordered_map>
#include <memory>

namespace storage
{
using node_id = std::size_t;

template <typename Node>
struct memory
{
    node_id new_node() const
    {
        static node_id counter = 0;
        return counter++;
    }

    std::unique_ptr<Node> load_node(const node_id & id) const
    {
        Node result = storage_.at(id);
        return std::move(result);
    }

    void delete_node(const node_id & id)
    {
        storage_.erase(id);
    }

    void write_node(const node_id & id, const std::unique_ptr<Node> & node)
    {
        storage_[id] = node;
    }

private:
    std::unordered_map<node_id, std::unique_ptr<Node> > storage_;
};
}
