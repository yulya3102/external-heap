#pragma once

#include <cstdint>
#include <deque>
#include <queue>
#include <set>

#include <boost/variant.hpp>
#include <boost/filesystem.hpp>

#include "storage/memory.h"

template <typename Id, typename T>
struct node_t
{
    Id id;
    std::deque<Id> children;

    void add(const T & x)
    {
        // TODO
    }
};

template <typename Id, typename T>
struct buffer_node_t : node_t<Id, T>
{
    std::queue<T> pending_add;

    void add(const T & x)
    {
        pending_add.push(x);
    }

    void flush()
    {
        while (!pending_add.empty())
        {
            auto x = pending_add.front();
            node_t<Id, T>::add(x);
            pending_add.pop();
        }
    }
};

template <typename Id, typename T>
struct leaf_t
{
    Id id;
    std::set<T> elements;
};

struct buffer_tree_t
{
    void add(std::int64_t x);

    template <typename OutIter>
    OutIter fetch_block(OutIter out);

    using node_id = int;
    using storage_node_t = boost::variant<buffer_node_t<node_id, std::int64_t>, leaf_t<node_id, std::int64_t> >;
    using buffer_storage_t = storage::memory<storage_node_t>;

private:
    leaf_t<node_id, std::int64_t> pop_left();

    buffer_storage_t storage;
};

template <typename OutIter>
OutIter buffer_tree_t::fetch_block(OutIter out)
{
    leaf_t<node_id, std::int64_t> leaf = pop_left();
    for (std::int64_t elem : leaf.elements)
    {
        *out = elem;
        ++out;
    }
    return out;
}
