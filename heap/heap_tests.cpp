#include "heap.h"

#include <gtest/gtest.h>
#include <utility>

TEST(small, descending)
{
    data::heap<std::uint64_t, std::uint64_t> heap(5);
    for (int i = 10; i >= 0; --i)
        heap.add(i, i);

    for (std::uint64_t i = 0; i <= 10; ++i)
        EXPECT_EQ(heap.remove_min(), std::make_pair(i, i));
}

TEST(small, random)
{
    std::default_random_engine generator;
    std::uniform_int_distribution<std::int64_t> distribution(1, 100);
    data::heap<std::uint64_t, std::uint64_t> heap(5);
    std::vector<std::pair<std::uint64_t, std::uint64_t>> elements;
    size_t size = 512;
    heap.add(100, 100);
    elements.push_back({100, 100});
    for (std::size_t i = 1; i < size; ++i)
    {
        auto x = distribution(generator);
        heap.add(x, x);
        elements.push_back({x, x});
    }

    std::vector<std::pair<std::uint64_t, std::uint64_t>> sorted;
    while (!heap.empty())
        sorted.push_back(heap.remove_min());
    std::sort(elements.begin(), elements.end());
    EXPECT_EQ(elements, sorted);
}

int main(int argc, char ** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
