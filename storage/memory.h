#pragma once

#include <unordered_map>
#include <memory>
#include <functional>

namespace storage
{
using node_id = std::uint64_t;

template <typename Serialized>
struct basic_storage
{
    virtual node_id new_node() const = 0;
    virtual std::shared_ptr<Serialized> load_node(const node_id & id) const = 0;
    virtual void delete_node(const node_id & id) = 0;
    virtual void write_node(const node_id & id, Serialized * node) = 0;
};

template <typename Node>
struct memory : basic_storage<Node>
{
    memory(std::function<Node *(Node *)> copy = [] (Node * node) -> Node * { return new Node(*node); })
        : copy_(copy)
    {}

    memory(const memory<Node> & other)
        : copy_(other.copy_)
    {
        for (auto & x : other.storage_)
            storage_[x.first].reset(copy_(x.second.get()));
    }

    virtual node_id new_node() const
    {
        static node_id counter = 0;
        return counter++;
    }

    virtual std::shared_ptr<Node> load_node(const node_id & id) const
    {
        return std::shared_ptr<Node>(copy_(storage_.at(id).get()));
    }

    virtual void delete_node(const node_id & id)
    {
        storage_.erase(id);
    }

    virtual void write_node(const node_id & id, Node * node)
    {
        storage_[id].reset(copy_(node));
    }

private:
    std::unordered_map<node_id, std::unique_ptr<Node>> storage_;
    std::function<Node * (Node *)> copy_;
};
}
