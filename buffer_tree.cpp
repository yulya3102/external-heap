#include "buffer_tree.h"

buffer_tree_t::buffer_tree_t()
{ }

void buffer_tree_t::add(std::int64_t x)
{
    // TODO
}

storage_t::any_node_t storage_t::load_node(const storage_t::node_id & id)
{

}

storage_t::node_id storage_t::root_node() const
{
    return root;
}

storage_t::leaf_t buffer_tree_t::node_visitor_t::operator () (const storage_t::leaf_t & leaf) const
{
    // for leaf:
    // remove leaf from filesystem
    // return leaf
}

storage_t::leaf_t buffer_tree_t::node_visitor_t::operator () (const storage_t::node_t & node) const
{
    // for node:
    // flush
    // get leaf from left node
    // if left node does not exist anymore
    // remove link to it
    // return received leaf
}

storage_t::leaf_t buffer_tree_t::pop_left()
{
    storage_t::any_node_t node = storage.load_node(storage.root_node());

    return boost::apply_visitor(node_visitor_t(), node);
}

