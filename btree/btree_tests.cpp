#include "btree.h"

#include <gtest/gtest.h>
#include <iterator>
#include <iostream>
#include <functional>

template <typename K, typename V, int t>
std::vector<std::pair<K, V> > from_tree(bptree::b_tree<K, V, t> & tree)
{
    std::vector<std::pair<K, V> > v;
    auto out = std::back_inserter(v);
    while (!tree.empty())
    {
        out = tree.remove_left_leaf(out);
    }

    return v;
}

TEST(btree, init)
{
    bptree::b_tree<int, int, 3> tree;
    for (size_t i = 0; i < 10; ++i)
        tree.add(i, i);
    for (size_t i = 20; i < 30; ++i)
        tree.add(i, i);
    for (size_t i = 19; i >= 10; --i)
        tree.add(i, i);

    std::vector<std::pair<int, int> > v = from_tree(tree);
    EXPECT_EQ(v.size(), 30);
    EXPECT_TRUE(std::is_sorted(v.begin(), v.end()));
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

    EXPECT_TRUE(std::is_sorted(v.begin(), v.end()));

    for (auto it = v.begin(); it != v.end(); ++it)
        tree.add(int(it->first), std::string(it->second));
    for (auto it = v.begin(); it != v.end(); ++it)
        tree.add(int(it->first), std::string(it->second));

    size_t expected_size = size + v.size();
    v.clear();

    v = from_tree(tree);

    EXPECT_EQ(v.size(), expected_size);

    for (std::size_t i = 0; i < expected_size - size; ++i)
        EXPECT_EQ(v[2 * i], v[2 * i + 1]);

    EXPECT_TRUE(std::is_sorted(v.begin(), v.end()));
}

TEST(btree, random)
{
    bptree::b_tree<int, std::string, 5> tree;
    std::vector<std::pair<int, std::string> > src, dest;

    std::default_random_engine generator;
    std::uniform_int_distribution<std::int64_t> distribution(1, 1000000);
    std::function<int()> random = std::bind(distribution, generator);

    std::size_t size = random() * 1000;
    for (std::size_t i = 0; i < size; ++i)
    {
        src.push_back(std::make_pair(random(), std::to_string(random())));
        tree.add(int(src.back().first), std::string(src.back().second));
    }

    dest = from_tree(tree);

    std::sort(src.begin(), src.end());

    std::vector<int> src_keys, dest_keys;
    auto src_keys_it = std::back_inserter(src_keys),
            dest_keys_it = std::back_inserter(dest_keys);
    std::transform(src.begin(), src.end(), src_keys_it, [](std::pair<int, std::string> x){ return x.first; });
    std::transform(dest.begin(), dest.end(), dest_keys_it, [](std::pair<int, std::string> x){ return x.first; });
    EXPECT_EQ(src_keys, dest_keys);
}

int main(int argc, char ** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
