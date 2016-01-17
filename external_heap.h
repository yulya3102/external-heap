#pragma once

#include <boost/optional.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

#include "buffer_tree.h"

struct heap_t
{
    struct options_t
    {
        options_t(size_t block_size = 4096,
                  size_t small_storage_blocks = 4);

        size_t block_size,
               element_size,
               small_storage_size,
               small_storage_elements;
    };

    heap_t();
    heap_t(const options_t & options);

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

    const options_t options;
};
