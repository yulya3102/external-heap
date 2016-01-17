#pragma once

#include <cstdint>


struct buffer_tree_t
{
    buffer_tree_t();
    void add(std::int64_t x);

    template <typename OutIter>
    OutIter fetch_block(OutIter out);
};

template <typename OutIter>
OutIter buffer_tree_t::fetch_block(OutIter out)
{
    // TODO
}
