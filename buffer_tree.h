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

    any_node_t load_node(const node_id & id) const;
    leaf_t pop_left(const node_id & id);

    struct node_t
    {
        std::vector<std::int64_t> pending_add;
        std::vector<node_id> children;

        void flush();
    };

    struct leaf_t
    {
        std::vector<std::int64_t> elements;
    };

    struct node_visitor_t : boost::static_visitor<buffer_tree_t::leaf_t>
    {
        buffer_tree_t::leaf_t operator () (const leaf_t & leaf) const;
        buffer_tree_t::leaf_t operator () (const node_t & node) const;
    };

    node_id root;
};

template <typename OutIter>
OutIter buffer_tree_t::fetch_block(OutIter out)
{
    leaf_t leaf = pop_left(root);
    for (std::int64_t elem : leaf.elements)
    {
        *out = elem;
        ++out;
    }
    return out;
}
