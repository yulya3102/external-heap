#pragma once

#include <cstdint>

#include <boost/variant.hpp>
#include <boost/filesystem.hpp>


struct buffer_tree_t
{
    buffer_tree_t();
    void add(std::int64_t x);

    template <typename OutIter>
    OutIter fetch_block(OutIter out);

private:
    struct node_t; struct leaf_t;
    using any_node_t = boost::variant<node_t, leaf_t>;
    using node_id = boost::filesystem::path;

    any_node_t load_node(const node_id & id);

    struct node_t
    {
        std::vector<std::int64_t> pending_add;
        std::vector<node_id> children;
    };

    struct leaf_t
    {
        std::vector<std::int64_t> elements;
    };

    node_id root;
};

template <typename OutIter>
OutIter buffer_tree_t::fetch_block(OutIter out)
{
    // TODO
}
