#pragma once

#include "btree_data.h"

#include <utils/undefined.h>

namespace bptree
{
std::string * serialize(detail::b_node_data<std::uint64_t, std::uint64_t> * data)
{
    undefined;
}

detail::b_node_data<std::uint64_t, std::uint64_t> * deserialize(std::string * serialized)
{
    undefined;
}
}
