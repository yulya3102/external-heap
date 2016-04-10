#pragma once

#include <btree/btree.h>
#include <storage/memory.h>
#include <utils/undefined.h>

#include <boost/optional.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace details
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
}

namespace data
{
template <typename Key, typename Value>
struct heap
{
    heap(std::size_t t)
        : big_elements(storage, t)
    {}

    void add(Key k, Value v)
    {
        undefined;
    }

    std::pair<Key, Value> remove_min()
    {
        undefined;
    }

private:
    void small_add(std::int64_t x);
    std::int64_t small_remove();
    bool is_small(std::int64_t x);
    void update_small_max();

    std::list<std::pair<Key, Value>> small_elements;
    boost::optional<Key> small_max;
    storage::memory<detail::b_node_data<Key, Value>> storage;
    bptree::b_tree<Key, Value> big_elements;
};
}
