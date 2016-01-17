#pragma once

#include <queue>
#include <deque>
#include <set>

#include <boost/variant.hpp>
#include <boost/filesystem.hpp>

struct storage_t
{
    struct node_t; struct leaf_t;
    using any_node_t = boost::variant<node_t, leaf_t>;
    using node_id = boost::filesystem::path;

    any_node_t load_node(const node_id & id) const;
    void delete_node(const node_id & id) const;
    node_id root_node() const;
    void write_node(const node_t & id) const;
    void write_node(const leaf_t & id) const;

    struct node_t
    {
        node_id id;
        std::queue<std::int64_t> pending_add;
        std::deque<node_id> children;

        void flush() const;
    };

    struct leaf_t
    {
        node_id id;
        std::set<std::int64_t> elements;
    };

private:
    node_id root;
};
