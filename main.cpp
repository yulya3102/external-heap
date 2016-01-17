#include <gtest/gtest.h>

#include "external_heap.h"

TEST(small, descending)
{
    heap_t heap;
    for (std::int64_t i = 10; i >= 0; --i)
        heap.add(i);

    for (std::int64_t i = 0; i <= 10; ++i)
        EXPECT_EQ(heap.remove_min(), i);
}

int main(int argc, char ** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
