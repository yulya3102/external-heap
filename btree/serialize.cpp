#include "serialize.h"

#include <boost/optional.hpp>
#include <queue>

namespace bptree
{
std::string * serialize(detail::b_node_data<std::uint64_t, std::uint64_t> * data)
{
    btree::BNode node;
    if (detail::b_leaf_data<std::uint64_t, std::uint64_t> * leaf_data
            = dynamic_cast<detail::b_leaf_data<std::uint64_t, std::uint64_t> *>(data))
    {
        btree::BLeaf * leaf = new btree::BLeaf;
        leaf->set_id(leaf_data->id_);
        leaf->set_level(leaf_data->level_);
        if (leaf_data->parent_)
            leaf->set_parent_id(*leaf_data->parent_);
        for (auto value : leaf_data->values_)
        {
            btree::KV * kv = leaf->add_value();
            kv->set_key(value.first);
            kv->set_value(value.second);
        }
        node.set_allocated_leaf(leaf);
    }
    else if (detail::b_buffer_data<std::uint64_t, std::uint64_t> * buffer_data
            = dynamic_cast<detail::b_buffer_data<std::uint64_t, std::uint64_t> *>(data))
    {
        btree::BBuffer * buffer = new btree::BBuffer;
        buffer->set_id(buffer_data->id_);
        buffer->set_level(buffer_data->level_);
        if (buffer_data->parent_)
            buffer->set_parent_id(*buffer_data->parent_);
        for (auto child : buffer_data->children_)
            buffer->add_child(child);
        for (auto key : buffer_data->keys_)
            buffer->add_key(key);
        node.set_allocated_buffer(buffer);
    }
    else
        throw std::logic_error("Unknown node type");

    return new std::string(node.SerializeAsString());
}

detail::b_node_data<std::uint64_t, std::uint64_t> * deserialize(std::string * serialized)
{
    btree::BNode node;
    auto r = node.ParseFromString(*serialized);
    if (!r)
        throw std::runtime_error("Error deserializing node");

    if (node.has_leaf())
    {
        const btree::BLeaf & leaf = node.leaf();
        boost::optional<storage::node_id> parent;
        if (leaf.has_parent_id())
            parent = leaf.parent_id();
        std::vector<std::pair<std::uint64_t, std::uint64_t>> values;
        for (auto v : leaf.value())
        {
            undefined;
        }
        return new detail::b_leaf_data<std::uint64_t, std::uint64_t>(
                    leaf.id(), parent, leaf.level(), values
        );
    }
    if (node.has_buffer())
    {
        const btree::BBuffer & buffer = node.buffer();
        boost::optional<storage::node_id> parent;
        if (buffer.has_parent_id())
            parent = buffer.parent_id();
        std::vector<std::uint64_t> keys;
        for (auto k : buffer.key())
        {
            undefined;
        }
        std::vector<storage::node_id> children;
        for (auto c : buffer.child())
        {
            undefined;
        }
        std::queue<std::pair<std::uint64_t, std::uint64_t>> pending;
        for (auto v : buffer.pending())
        {
            undefined;
        }
        return new detail::b_buffer_data<std::uint64_t, std::uint64_t>(
                    buffer.id(), parent, buffer.level(),
                    keys, children, pending
        );
    }

    throw std::runtime_error("Unknown serialized node type");
}
}
