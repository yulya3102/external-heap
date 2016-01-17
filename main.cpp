#include <gtest/gtest.h>

#include "external_heap.h"

TEST(empty, size)
{
    heap_t heap;
    EXPECT_EQ(heap.size(), 0);
}

int main(int argc, char ** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
