#include "btree.h"

#include <gtest/gtest.h>
#include <iterator>
#include <iostream>
#include <functional>

template <typename K, typename V, typename Serialized>
std::vector<std::pair<K, V>> from_tree(bptree::b_tree<K, V, Serialized> & tree)
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
    storage::memory<std::string> mem;
    bptree::b_tree<std::uint64_t, std::uint64_t> tree(mem, 3);
    for (size_t i = 0; i < 10; ++i)
        tree.add(i, i);
    for (size_t i = 20; i < 30; ++i)
        tree.add(i, i);
    for (size_t i = 19; i >= 10; --i)
        tree.add(i, i);

    std::vector<std::pair<std::uint64_t, std::uint64_t> > v = from_tree(tree);
    EXPECT_EQ(v.size(), 30);
    EXPECT_TRUE(std::is_sorted(v.begin(), v.end()));
}

TEST(btree, repeating)
{
    storage::memory<std::string> mem;
    bptree::b_tree<std::uint64_t, std::uint64_t> tree(mem, 3);

    std::default_random_engine generator;
    std::uniform_int_distribution<std::int64_t> distribution(1, 1000);
    std::function<int()> random = std::bind(distribution, generator);

    std::size_t size = random() * 1000;
    for (std::size_t i = 0; i < size; ++i)
        tree.add(i, i);

    std::vector<std::pair<std::uint64_t, std::uint64_t> > v;
    auto v_back = std::back_inserter(v);
    v_back = tree.remove_left_leaf(v_back);

    EXPECT_TRUE(std::is_sorted(v.begin(), v.end()));

    for (auto it = v.begin(); it != v.end(); ++it)
        tree.add(int(it->first), (it->second));
    for (auto it = v.begin(); it != v.end(); ++it)
        tree.add(int(it->first), (it->second));

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
    storage::memory<std::string> mem;
    bptree::b_tree<std::uint64_t, std::uint64_t> tree(mem, 5);
    std::vector<std::pair<std::uint64_t, std::uint64_t> > src, dest;

    std::default_random_engine generator;
    std::uniform_int_distribution<std::int64_t> distribution(1, 1000000);
    std::function<int()> random = std::bind(distribution, generator);

    std::size_t size = random() * 1000;
    for (std::size_t i = 0; i < size; ++i)
    {
        src.push_back(std::make_pair(random(), (random())));
        tree.add(int(src.back().first),(src.back().second));
    }

    dest = from_tree(tree);

    std::sort(src.begin(), src.end());

    std::vector<int> src_keys, dest_keys;
    auto src_keys_it = std::back_inserter(src_keys),
            dest_keys_it = std::back_inserter(dest_keys);
    std::transform(src.begin(), src.end(), src_keys_it, [](auto x){ return x.first; });
    std::transform(dest.begin(), dest.end(), dest_keys_it, [](auto x){ return x.first; });
    EXPECT_EQ(src_keys, dest_keys);
}

TEST(btree, independent)
{
    std::default_random_engine generator;
    std::uniform_int_distribution<std::int64_t> distribution(1, 1000);
    std::function<int()> random = std::bind(distribution, generator);

    storage::memory<std::string> mem1;
    bptree::b_tree<std::uint64_t, std::uint64_t> tree1(mem1, 6);
    std::size_t size1 = random() * 100;
    for (std::size_t i = 0; i < size1; ++i)
    {
        auto x = random();
        tree1.add(x, x);
    }

    tree1.flush_cache();
    storage::memory<std::string> mem2(mem1);
    bptree::b_tree<std::uint64_t, std::uint64_t> tree2(mem2, 6, tree1.root_id());
    std::size_t size2 = random() * 10;
    for (std::size_t i = 0; i < size2; ++i)
    {
        auto x = random() + 1000;
        tree2.add(x, x);
    }

    std::vector<std::pair<std::uint64_t, std::uint64_t> > v;
    auto v_out = std::back_inserter(v);
    for (std::size_t i = 0; i < size1 / (6 - 1) + 1; ++i)
        v_out = tree2.remove_left_leaf(v_out);

    std::vector<std::pair<std::uint64_t, std::uint64_t> > v1 = from_tree(tree1), v2 = from_tree(tree2);
    EXPECT_EQ(v1.size(), size1);
    EXPECT_EQ(v2.size(), size1 + size2 - v.size());

    for (auto x : v1)
        EXPECT_EQ(std::find(v2.begin(), v2.end(), x), v2.end());
}

int main(int argc, char ** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
