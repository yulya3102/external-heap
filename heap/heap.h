#pragma once

#include <btree/btree.h>
#include <storage/directory.h>
#include <utils/undefined.h>

#include <vector>
#include <algorithm>
#include <utility>
#include <limits>

namespace data
{
template <typename Key, typename Value>
struct heap
{
    heap(std::size_t t, Key small_max = std::numeric_limits<Key>::max())
        : small_size(2 * t)
        , small_max(small_max)
        , storage("storage")
        , big(storage, t)
    {}

    void add(Key k, Value v)
    {
        if (k < small_max)
            small_add(k, v);
        else
            big_add(k, v);
    }

    std::pair<Key, Value> remove_min()
    {
        if (small.empty())
        {
            if (big.empty())
                throw std::runtime_error("Trying to remove minimal element from empty heap");
            auto out = std::back_inserter(small);
            big.remove_left_leaf(out);
            small_max = small.back().first;
        }

        auto result = small.front();
        small.pop_front();
        return result;
    }

    bool empty()
    {
        return small.empty() && big.empty();
    }

private:
    void small_add(Key k, Value v)
    {
        if (small.size() == small_size)
        {
            for (size_t i = 0; i < small_size / 2; ++i)
            {
                auto max = small.back();
                big_add(max.first, max.second);
                small.pop_back();
                small_max = max.first;
            }

            // k can be > small_max now
            add(k, v);
        }
        else
        {
            auto x = std::make_pair(k, v);
            auto it = std::lower_bound(small.begin(), small.end(), x);
            small.insert(it, x);
        }
    }

    void big_add(Key k, Value v)
    {
        big.add(k, v);
    }

    std::size_t small_size;
    Key small_max;
    std::list<std::pair<Key, Value>> small;
    storage::directory<std::string> storage;
    bptree::b_tree<Key, Value> big;
};
}
