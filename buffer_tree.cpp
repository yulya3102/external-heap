#include "buffer_tree.h"

namespace
{
struct node_add_visitor_t : boost::static_visitor<void>
{
    node_add_visitor_t(const storage_t & storage, std::int64_t x)
        : storage(storage)
        , x(x)
    {}

    void operator () (storage_t::leaf_t & leaf) const
    {
        leaf.elements.insert(x);
        storage.write_node(leaf);
    }

    void operator () (storage_t::node_t & node) const
    {
        node.pending_add.push(x);
        storage.write_node(node);
    }

private:
    const storage_t & storage;
    std::int64_t x;
};
}

void buffer_tree_t::add(std::int64_t x)
{
    storage_t::any_node_t node = storage.load_node(storage.root_node());
    boost::apply_visitor(node_add_visitor_t(storage, x), node);
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

void storage_t::write_node(const storage_t::node_t & id) const
{

}

void storage_t::write_node(const storage_t::leaf_t & id) const
{

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

    visitor_result_t operator () (const storage_t::leaf_t & leaf) const
    {
        storage.delete_node(leaf.id);
        return { leaf, true };
    }

    visitor_result_t operator () (storage_t::node_t & node) const
    {
        node.flush();

        storage_t::any_node_t first_child = storage.load_node(node.children.front());
        visitor_result_t res = boost::apply_visitor(node_visitor_t(storage), first_child);
        if (res.was_removed)
            node.children.pop_front();

        bool was_removed = false;
        if (node.children.empty())
        {
            storage.delete_node(node.id);
            was_removed = true;
        }
        else
            storage.write_node(node);

        return { res.leaf, was_removed };
    }

private:
    const storage_t & storage;
};
}

storage_t::leaf_t buffer_tree_t::pop_left()
{
    storage_t::any_node_t node = storage.load_node(storage.root_node());

    return boost::apply_visitor(node_visitor_t(storage), node).leaf;
}



void storage_t::node_t::flush() const
{

}
