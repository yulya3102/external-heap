#pragma once

#include <queue>
#include <deque>
#include <set>

#include <boost/variant.hpp>
#include <boost/filesystem.hpp>

template <typename Id>
struct node_t
{
    Id id;
    std::deque<Id> children;
};

template <typename Id>
struct buffer_node_t : node_t<Id>
{
    std::queue<std::int64_t> pending_add;

    void flush() const
    {

    }
};

template <typename Id>
struct leaf_t
{
    Id id;
    std::set<std::int64_t> elements;
};

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
