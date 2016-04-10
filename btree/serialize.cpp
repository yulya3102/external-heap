#include "serialize.h"

namespace bptree
{
std::string * serialize(detail::b_node_data<std::uint64_t, std::uint64_t> * data)
{
    if (detail::b_leaf_data<std::uint64_t, std::uint64_t> * leaf_data
            = dynamic_cast<detail::b_leaf_data<std::uint64_t, std::uint64_t> *>(data))
    {
        btree::BLeaf leaf;
        leaf.set_id(leaf_data->id_);
        leaf.set_level(leaf_data->level_);
        if (leaf_data->parent_)
            leaf.set_parent_id(*leaf_data->parent_);
        for (auto value : leaf_data->values_)
        {
            btree::KV * kv = leaf.add_value();
            kv->set_key(value.first);
            kv->set_value(value.second);
        }
        return new std::string(leaf.SerializeAsString());
    }
    else if (detail::b_buffer_data<std::uint64_t, std::uint64_t> * buffer_data
            = dynamic_cast<detail::b_buffer_data<std::uint64_t, std::uint64_t> *>(data))
    {
        btree::BBuffer buffer;
        buffer.set_id(buffer_data->id_);
        buffer.set_level(buffer_data->level_);
        if (buffer_data->parent_)
            buffer.set_parent_id(*buffer_data->parent_);
        for (auto child : buffer_data->children_)
            buffer.add_child(child);
        for (auto key : buffer_data->keys_)
            buffer.add_key(key);
        return new std::string(buffer.SerializeAsString());
    }
    else
        throw std::logic_error("Unknown node type");
}

detail::b_node_data<std::uint64_t, std::uint64_t> * deserialize(std::string * serialized)
{
    undefined;
}
}
