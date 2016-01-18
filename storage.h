#pragma once

#include <queue>
#include <deque>
#include <set>

#include <boost/variant.hpp>
#include <boost/filesystem.hpp>

template <typename Storage>
struct node_t
{
    using node_id = typename Storage::node_id;

    node_id id;
    std::queue<std::int64_t> pending_add;
    std::deque<node_id> children;

    void flush() const
    {

    }
};

template <typename Storage>
struct leaf_t
{
    using node_id = typename Storage::node_id;

    node_id id;
    std::set<std::int64_t> elements;
};

struct storage_t
{
    using any_node_t = boost::variant<node_t<storage_t>, leaf_t<storage_t> >;
    using node_id = boost::filesystem::path;

    any_node_t load_node(const node_id & id) const;
    void delete_node(const node_id & id) const;
    node_id root_node() const;
    void write_node(const node_t<storage_t> & id) const;
    void write_node(const leaf_t<storage_t> & id) const;

private:
    node_id root;
};
