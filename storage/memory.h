#pragma once

#include <unordered_map>

namespace storage
{
using node_id = std::size_t;

template <typename Node>
struct memory
{
    memory() {}

    memory(const memory<Node> & other)
    {
        for (auto x : other.storage_)
            storage_[x.first] = x.second->copy(*this);
    }

    node_id new_node() const
    {
        static node_id counter = 0;
        return counter++;
    }

    Node * load_node(const node_id & id) const
    {
        return storage_.at(id)->copy(*this);

    }

    void delete_node(const node_id & id)
    {
        Node * deleted = storage_.at(id);
        storage_.erase(id);
        delete deleted;
    }

    void write_node(const node_id & id, Node * node)
    {
        auto old_it = storage_.find(id);
        Node * old = nullptr;
        if (old_it != storage_.end())
            old = old_it->second;
        storage_[id] = node->copy(*this);
        delete old;
    }

private:
    std::unordered_map<node_id, Node *> storage_;
};
}
