#include "buffer_tree.h"

void buffer_tree_t::add(std::int64_t x)
{
    // TODO
}

storage_t::any_node_t storage_t::load_node(const storage_t::node_id & id) const
{

}

void storage_t::delete_node(const storage_t::node_id & id) const
{

}

storage_t::node_id storage_t::root_node() const
{
    return root;
}

namespace
{
struct visitor_result_t
{
    storage_t::leaf_t leaf;
    bool was_removed;
};

struct node_visitor_t : boost::static_visitor<visitor_result_t>
{
    node_visitor_t(const storage_t & storage)
        : storage(storage)
    {}

    visitor_result_t operator () (const storage_t::leaf_t & leaf) const;
    visitor_result_t operator () (storage_t::node_t & node) const;

private:
    const storage_t & storage;
};

visitor_result_t node_visitor_t::operator () (const storage_t::leaf_t & leaf) const
{
    storage.delete_node(leaf.id);
    return { leaf, true };
}

visitor_result_t node_visitor_t::operator () (storage_t::node_t & node) const
{
    node.flush();

    storage_t::any_node_t first_child = storage.load_node(node.children.front());
    visitor_result_t res = boost::apply_visitor(node_visitor_t(storage), first_child);
    if (res.was_removed)
        node.children.pop_front();

    // TODO: check if there are no children left
    bool was_removed = false;

    // TODO: update node on filesystem

    return { res.leaf, was_removed };
}
}

storage_t::leaf_t buffer_tree_t::pop_left()
{
    storage_t::any_node_t node = storage.load_node(storage.root_node());

    return boost::apply_visitor(node_visitor_t(storage), node).leaf;
}



void storage_t::node_t::flush() const
{

}
