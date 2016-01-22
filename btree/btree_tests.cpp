#include "btree.h"

#include <gtest/gtest.h>

TEST(btree, init)
{
    bptree::b_tree<int, int, 3> tree;
    for (size_t i = 0; i < 10; ++i)
        tree.add(i, i);
    for (size_t i = 20; i >= 10; --i)
        tree.add(i, i);
}

int main(int argc, char ** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
