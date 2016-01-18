#pragma once

#include <cstdint>
#include <deque>
#include <queue>
#include <set>

#include <boost/variant.hpp>
#include <boost/filesystem.hpp>

#include "storage.h"

struct buffer_tree_t
{
    void add(std::int64_t x);

    template <typename OutIter>
    OutIter fetch_block(OutIter out);

private:
    leaf_t<storage_t::node_id> pop_left();

    storage_t storage;
};

template <typename OutIter>
OutIter buffer_tree_t::fetch_block(OutIter out)
{
    leaf_t<storage_t::node_id> leaf = pop_left();
    for (std::int64_t elem : leaf.elements)
    {
        *out = elem;
        ++out;
    }
    return out;
}
