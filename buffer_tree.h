#pragma once

#include <cstdint>
#include <deque>
#include <queue>
#include <set>

#include <boost/variant.hpp>
#include <boost/filesystem.hpp>

#include "storage.h"

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

struct buffer_tree_t
{
    void add(std::int64_t x);

    template <typename OutIter>
    OutIter fetch_block(OutIter out);

    using node_id = boost::filesystem::path;
    using storage_node_t = boost::variant<buffer_node_t<node_id>, leaf_t<node_id> >;
    using buffer_storage_t = storage_t<storage_node_t>;

private:
    leaf_t<node_id> pop_left();

    buffer_storage_t storage;
};

template <typename OutIter>
OutIter buffer_tree_t::fetch_block(OutIter out)
{
    leaf_t<node_id> leaf = pop_left();
    for (std::int64_t elem : leaf.elements)
    {
        *out = elem;
        ++out;
    }
    return out;
}
