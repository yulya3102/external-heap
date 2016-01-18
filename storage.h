#pragma once

#include <boost/filesystem.hpp>

template <typename Node>
struct storage_t
{
    using node_id = boost::filesystem::path;

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
