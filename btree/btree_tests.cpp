#include "btree.h"

#include <gtest/gtest.h>
#include <iterator>
#include <iostream>
#include <functional>

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

TEST(btree, repeating)
{
    bptree::b_tree<int, std::string, 3> tree;

    std::default_random_engine generator;
    std::uniform_int_distribution<std::int64_t> distribution(1, 1000);
    std::function<int()> random = std::bind(distribution, generator);

    std::size_t size = random() * 1000;
    for (std::size_t i = 0; i < size; ++i)
        tree.add(i, std::to_string(i));

    std::vector<std::pair<int, std::string> > v;
    auto v_back = std::back_inserter(v);
    v_back = tree.remove_left_leaf(v_back);

    for (auto it1 = v.begin(), it2 = v.begin() + 1; it2 != v.end(); ++it1, ++it2)
        EXPECT_LE(*it1, *it2);

    for (auto it = v.begin(); it != v.end(); ++it)
        tree.add(int(it->first), std::string(it->second));
    for (auto it = v.begin(); it != v.end(); ++it)
        tree.add(int(it->first), std::string(it->second));

    size_t expected_size = size + v.size();
    v.clear();

    while (!tree.empty())
        v_back = tree.remove_left_leaf(v_back);

    EXPECT_EQ(v.size(), expected_size);

    EXPECT_EQ(v[0], v[1]);

    for (auto it1 = v.begin(), it2 = v.begin() + 1; it2 != v.end(); ++it1, ++it2)
        EXPECT_LE(*it1, *it2);
}

int main(int argc, char ** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
