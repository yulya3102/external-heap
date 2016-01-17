#pragma once

#include <boost/optional.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

#include "buffer_tree.h"

#define BLOCK_SIZE 4096
#define ELEMENT_SIZE 8
#define SMALL_STORAGE_SIZE (4 * BLOCK_SIZE)
#define SMALL_STORAGE_ELEMENTS (SMALL_STORAGE_SIZE / ELEMENT_SIZE)

struct heap_t
{
    heap_t();
    void add(std::int64_t x);
    std::int64_t remove_min();

private:
    void small_add(std::int64_t x);
    std::int64_t small_remove();
    bool is_small(std::int64_t x);
    void update_small_max();

    std::vector<std::int64_t> small_elements;
    boost::optional<std::int64_t> small_max;
    buffer_tree_t big_elements;
};
