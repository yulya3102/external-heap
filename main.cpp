#include "external_heap.h"

#include <gtest/gtest.h>
#include <utility>

TEST(small, descending)
{
    data::heap<int, int> heap(5);
    for (int i = 10; i >= 0; --i)
        heap.add(i, i);

    for (int i = 0; i <= 10; ++i)
        EXPECT_EQ(heap.remove_min(), std::make_pair(i, i));
}

TEST(small, random)
{
    std::default_random_engine generator;
    std::uniform_int_distribution<std::int64_t> distribution(1, 100);
    data::heap<int, int> heap(5);
    size_t elements = 512;
    heap.add(100, 100);
    for (std::size_t i = 1; i < elements; ++i)
        heap.add(distribution(generator), i);

    std::vector<std::pair<int, int>> sorted;
    for (std::size_t i = 0; i < elements; ++i)
        sorted.push_back(heap.remove_min());
    EXPECT_TRUE(std::is_sorted(sorted.begin(), sorted.end()));
}

int main(int argc, char ** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
