#pragma once

namespace storage
{
template <typename Node>
struct memory
{
    using node_id = int;

    Node load_node(const node_id & id) const
    {

    }

    void delete_node(const node_id & id) const
    {

    }

    node_id root_node() const
    {

    }

    void write_node(const node_id & id, const Node & node) const
    {

    }

private:
    node_id root;
};
}
