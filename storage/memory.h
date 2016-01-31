#pragma once

#include <unordered_map>

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

    Node * load_node(const node_id & id) const
    {
        return storage_.at(id)->copy();

    }

    void delete_node(const node_id & id)
    {
        Node * deleted = storage_.at(id);
        storage_.erase(id);
        delete deleted;
    }

    void write_node(const node_id & id, Node * node)
    {
        storage_[id] = node->copy();
    }

private:
    std::unordered_map<node_id, Node *> storage_;
};
}
