#pragma once

#include "basic_storage.h"
#include "node_id.h"

#include <memory>

namespace storage
{
template <typename Serialized>
struct basic_storage
{
    virtual node_id new_node() const = 0;
    virtual std::shared_ptr<Serialized> load_node(const node_id & id) const = 0;
    virtual void delete_node(const node_id & id) = 0;
    virtual void write_node(const node_id & id, Serialized * node) = 0;
};

}
