#pragma once

#include <cstdint>
#include <deque>
#include <queue>

#include <boost/variant.hpp>
#include <boost/filesystem.hpp>

struct storage_t
{
    struct node_t; struct leaf_t;
    using any_node_t = boost::variant<node_t, leaf_t>;
    using node_id = boost::filesystem::path;

    any_node_t load_node(const node_id & id) const;
    void delete_node(const node_id & id) const;
    node_id root_node() const;

    struct node_t
    {
        node_id id;
        std::queue<std::int64_t> pending_add;
        std::deque<node_id> children;

        void flush() const;
    };

    struct leaf_t
    {
        node_id id;
        std::vector<std::int64_t> elements;
    };

private:
    node_id root;
};

struct buffer_tree_t
{
    void add(std::int64_t x);

    template <typename OutIter>
    OutIter fetch_block(OutIter out);

private:
    storage_t::leaf_t pop_left();

    storage_t storage;
};

template <typename OutIter>
OutIter buffer_tree_t::fetch_block(OutIter out)
{
    storage_t::leaf_t leaf = pop_left();
    for (std::int64_t elem : leaf.elements)
    {
        *out = elem;
        ++out;
    }
    return out;
}
