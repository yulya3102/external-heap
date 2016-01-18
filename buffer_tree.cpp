#include "buffer_tree.h"

#include <boost/variant.hpp>

namespace
{
/*
struct node_add_visitor_t : boost::static_visitor<void>
{
    node_add_visitor_t(buffer_tree_t::buffer_storage_t & storage, std::int64_t x)
        : storage(storage)
        , x(x)
    {}

    void operator () (leaf_t<buffer_tree_t::node_id, std::int64_t> & leaf)
    {
        leaf.add(x);
        storage.write_node(leaf.id(), leaf);
    }

    void operator () (buffer_node_t<buffer_tree_t::node_id, std::int64_t> & node)
    {
        node.add(x);
        storage.write_node(node.id(), node);
    }

private:
    buffer_tree_t::buffer_storage_t & storage;
    std::int64_t x;
};
*/
}

void buffer_tree_t::add(std::int64_t x)
{
    storage_node_t node = storage.load_node(root_);
    // TODO: add x to node
}

namespace
{
/*
struct visitor_result_t
{
    leaf_t<buffer_tree_t::node_id, std::int64_t> leaf;
    bool was_removed;
};

struct node_visitor_t : boost::static_visitor<visitor_result_t>
{
    node_visitor_t(buffer_tree_t::buffer_storage_t & storage)
        : storage(storage)
    {}

    visitor_result_t operator () (const leaf_t<buffer_tree_t::node_id, std::int64_t> & leaf)
    {
        storage.delete_node(leaf.id());
        return { leaf, true };
    }

    visitor_result_t operator () (buffer_node_t<buffer_tree_t::node_id, std::int64_t> & node)
    {
        node.flush();

        buffer_tree_t::storage_node_t first_child = storage.load_node(node.children.front());
        node_visitor_t visitor(storage);
        visitor_result_t res = boost::apply_visitor(visitor, first_child);
        if (res.was_removed)
            node.children.pop_front();

        bool was_removed = false;
        if (node.children.empty())
        {
            storage.delete_node(node.id());
            was_removed = true;
        }
        else
            storage.write_node(node.id(), node);

        return { res.leaf, was_removed };
    }

private:
    buffer_tree_t::buffer_storage_t & storage;
};
*/
}

leaf_t<buffer_tree_t::node_id, std::int64_t> buffer_tree_t::pop_left()
{
    buffer_tree_t::storage_node_t node = storage.load_node(root_);

    // TODO: pop left leaf from node
}
