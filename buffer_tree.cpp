#include "buffer_tree.h"

buffer_tree_t::buffer_tree_t()
    : root("root")
{ }

void buffer_tree_t::add(std::int64_t x)
{
    // TODO
}

buffer_tree_t::any_node_t buffer_tree_t::load_node(const buffer_tree_t::node_id & id) const
{

}

buffer_tree_t::leaf_t buffer_tree_t::node_visitor_t::operator () (const leaf_t & leaf) const
{
    // for leaf:
    // remove leaf from filesystem
    // return leaf
}

buffer_tree_t::leaf_t buffer_tree_t::node_visitor_t::operator () (const node_t & node) const
{
    // for node:
    // flush
    // get leaf from left node
    // if left node does not exist anymore
    // remove link to it
    // return received leaf
}

buffer_tree_t::leaf_t buffer_tree_t::pop_left(const buffer_tree_t::node_id & id)
{
    any_node_t node = load_node(id);

    boost::apply_visitor(node_visitor_t(), node);
}

