#pragma once

#include "memory.h"

#include <memory>

namespace storage
{
template <typename Node>
struct cache
{
    cache(memory<Node> & storage)
        : storage_(storage)
    {}

    node_id new_node() const
    {
        return storage_.new_node();
    }

    std::shared_ptr<Node> load_node(const node_id & id) const
    {
        return storage_.load_node(id);
    }

    void delete_node(const node_id & id)
    {
        storage_.delete_node(id);
    }

    void write_node(const node_id & id, Node * node)
    {
        storage_.write_node(id, node);
    }

private:
    memory<Node> & storage_;
};
}
