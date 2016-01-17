#include "external_heap.h"

#include <boost/none.hpp>

#include <algorithm>

heap_t::heap_t()
{

}

void heap_t::add(int64_t x)
{
    if (is_small(x))
        small_add(x);
    else
    {
        // TODO: add x to "big" elements
    }
}

int64_t heap_t::remove_min()
{
    if (small_elements.empty())
    {
        // TODO: take from "big" elements
    }
    else
        return small_remove();
}

void heap_t::small_add(int64_t x)
{
    small_elements.push_back(-x);
    std::push_heap(small_elements.begin(), small_elements.end());

    if (small_elements.size() >= SMALL_STORAGE_ELEMENTS)
    {
        // TODO: move small_max to "big" elements
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
