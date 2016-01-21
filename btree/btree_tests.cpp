#include "btree.h"

#include <gtest/gtest.h>

TEST(btree, init)
{
    bptree::b_tree<int, int, 2> tree;
}

int main(int argc, char ** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
