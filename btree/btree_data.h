#pragma once

#include <storage/node_id.h>

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
    std::uint64_t level_;

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

    b_leaf_data(const storage::node_id & id)
        : b_node_data<Key, Value>(id, boost::none, 0)
    {}

    b_leaf_data(const storage::node_id & id,
                const boost::optional<storage::node_id> & parent,
                std::size_t level,
                const std::vector<std::pair<Key, Value>> & values)
        : b_node_data<Key, Value>(id, parent, level)
        , values_(values)
    {}

    b_leaf_data() {}

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

    b_internal_data(const std::vector<Key> & keys,
                    const std::vector<storage::node_id> & children)
        : keys_(keys)
        , children_(children)
    {}

    b_internal_data() {}
};

template <typename Key, typename Value>
struct b_buffer_data : virtual b_internal_data<Key, Value>
{
    std::queue<std::pair<Key, Value> > pending_add_;

    b_buffer_data(const storage::node_id & id,
                  std::size_t level)
        : b_node_data<Key, Value>(id, boost::none, level)
    {}

    b_buffer_data(const storage::node_id & id,
                  const boost::optional<storage::node_id> & parent,
                  std::size_t level,
                  const std::vector<Key> & keys,
                  const std::vector<storage::node_id> & children,
                  const std::queue<std::pair<Key, Value>> & pending)
        : b_node_data<Key, Value>(id, parent, level)
        , b_internal_data<Key, Value>(keys, children)
        , pending_add_(pending)
    {}

    b_buffer_data() {}

    virtual b_buffer_data * copy_data() const
    {
        return new b_buffer_data(*this);
    }
};
}
