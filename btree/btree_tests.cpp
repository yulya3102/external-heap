#include "btree.h"

#include <gtest/gtest.h>
#include <iterator>
#include <iostream>

TEST(btree, init)
{
    bptree::b_tree<int, int, 3> tree;
    for (size_t i = 0; i < 10; ++i)
        tree.add(i, i);
    for (size_t i = 20; i < 30; ++i)
        tree.add(i, i);
    for (size_t i = 19; i >= 10; --i)
        tree.add(i, i);

    std::vector<std::pair<int, int> > v;
    auto out = std::back_inserter(v);
    while (!tree.empty())
    {
        out = tree.remove_left_leaf(out);
    }
}

int main(int argc, char ** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
