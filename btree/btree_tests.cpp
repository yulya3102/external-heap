#include "btree.h"

#include <gtest/gtest.h>

TEST(btree, init)
{
    bptree::b_internal_node<int, int, 2> node;
}

int main(int argc, char ** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
