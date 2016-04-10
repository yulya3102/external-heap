#pragma once

#include "basic_storage.h"

#include <utils/undefined.h>

namespace storage
{
template <typename Serialized>
struct directory : basic_storage<Serialized>
{
    virtual node_id new_node() const
    {
        undefined;
    }

    virtual std::shared_ptr<Serialized> load_node(const node_id & id) const
    {
        undefined;
    }

    virtual void delete_node(const node_id & id)
    {
        undefined;
    }

    virtual void write_node(const node_id & id, Serialized * node)
    {
        undefined;
    }
};
}
