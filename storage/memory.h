#pragma once

#include <unordered_map>
#include <memory>

namespace storage
{
using node_id = std::size_t;

template <typename Node>
struct memory
{
    memory() {}

    memory(const memory<Node> & other)
    {
        for (auto & x : other.storage_)
            storage_[x.first].reset(x.second->copy(*this));
    }

    node_id new_node() const
    {
        static node_id counter = 0;
        return counter++;
    }

    std::shared_ptr<Node> load_node(const node_id & id) const
    {
        return std::shared_ptr<Node>(storage_.at(id)->copy(*this));
    }

    void delete_node(const node_id & id)
    {
        storage_.erase(id);
    }

    void write_node(const node_id & id, Node * node)
    {
        storage_[id].reset(node->copy(*this));
    }

private:
    std::unordered_map<node_id, std::unique_ptr<Node>> storage_;
};
}
