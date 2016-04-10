#pragma once

#include <storage/cache.h>

#include <boost/optional.hpp>
#include <exception>
#include <vector>
#include <queue>

namespace detail
{
template <typename Key, typename Value>
struct b_node_data
{
    storage::node_id id_;
    boost::optional<storage::node_id> parent_;
    std::size_t level_;

    b_node_data()
    {
        throw std::logic_error(
            "b_node_data(): this constructor should never be called"
        );
    }

    b_node_data(const storage::node_id & id,
                const boost::optional<storage::node_id> & parent,
                std::size_t level)
        : id_(id)
        , parent_(parent)
        , level_(level)
    {}

    virtual b_node_data * copy_data() const = 0;

    virtual ~b_node_data() = default;
};

template <typename Key, typename Value>
struct b_leaf_data : virtual b_node_data<Key, Value>
{
    std::vector<std::pair<Key, Value>> values_;

    virtual b_leaf_data * copy_data() const
    {
        return new b_leaf_data(*this);
    }
};

template <typename Key, typename Value>
struct b_internal_data : virtual b_node_data<Key, Value>
{
    std::vector<Key> keys_;
    std::vector<storage::node_id> children_;
};

template <typename Key, typename Value>
struct b_buffer_data : virtual b_internal_data<Key, Value>
{
    std::queue<std::pair<Key, Value> > pending_add_;

    virtual b_buffer_data * copy_data() const
    {
        return new b_buffer_data(*this);
    }
};
}
