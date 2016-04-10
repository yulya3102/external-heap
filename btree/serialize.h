#pragma once

#include "serialize/btree.pb.h"
#include "btree_data.h"

#include <utils/undefined.h>

#include <exception>

namespace bptree
{
std::string * serialize(detail::b_node_data<std::uint64_t, std::uint64_t> * data);
detail::b_node_data<std::uint64_t, std::uint64_t> * deserialize(std::string * serialized);
}
