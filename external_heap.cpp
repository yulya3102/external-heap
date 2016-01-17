#include "external_heap.h"

#include <boost/none.hpp>

#include <algorithm>

heap_t::heap_t(const heap_t::options_t & options)
    : options(options)
{}

heap_t::heap_t() {}

void heap_t::add(int64_t x)
{
    if (is_small(x))
        small_add(x);
    else
        big_elements.add(-x);
}

int64_t heap_t::remove_min()
{
    if (small_elements.empty())
    {
        big_elements.fetch_block(std::back_inserter(small_elements));
        std::make_heap(small_elements.begin(), small_elements.end());
    }
    return small_remove();
}

void heap_t::small_add(int64_t x)
{
    small_elements.push_back(-x);
    std::push_heap(small_elements.begin(), small_elements.end());

    if (small_elements.size() >= options.small_storage_elements)
    {
        big_elements.add(*small_max);
        small_elements.erase(std::find(small_elements.begin(), small_elements.end(), small_max));
    }

    update_small_max();
}

int64_t heap_t::small_remove()
{
    int64_t elem = -small_elements.front();
    std::pop_heap(small_elements.begin(), small_elements.end());
    small_elements.pop_back();

    update_small_max();

    return elem;
}

bool heap_t::is_small(int64_t x)
{
    if (small_max)
        return x <= small_max;
    return true;
}

void heap_t::update_small_max()
{
    // TODO: optimize this
    if (small_elements.empty())
        small_max = boost::none;
    else
        small_max = -*std::min_element(small_elements.begin(), small_elements.end());
}


heap_t::options_t::options_t(size_t block_size, size_t small_storage_blocks)
    : block_size(block_size)
    , element_size(sizeof(std::int64_t))
    , small_storage_size(block_size * small_storage_blocks)
    , small_storage_elements(small_storage_size / element_size)
{}
