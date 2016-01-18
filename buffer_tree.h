#pragma once

#include <cstdint>
#include <deque>
#include <queue>
#include <set>

#include <boost/variant.hpp>
#include <boost/filesystem.hpp>

#include "storage/memory.h"

template <typename Id>
struct any_node_t
{
    Id id() const
    {
        return id_;
    }

private:
    Id id_;
};

template <typename Id, typename T>
struct node_t : any_node_t<Id>
{
    std::deque<Id> children;

    void add(const T & x)
    {
        // TODO
    }
};

template <typename Id, typename T>
struct buffer_node_t : node_t<Id, T>
{
    void add(const T & x)
    {
        pending_add.push(x);
    }

    void flush()
    {
        while (!pending_add.empty())
        {
            auto x = pending_add.front();
            node_t<Id, T>::add(x);
            pending_add.pop();
        }
    }

private:
    std::queue<T> pending_add;
};

template <typename Id, typename T>
struct leaf_t : any_node_t<Id>
{
    template <typename OutIter>
    OutIter & write_elements(OutIter & out)
    {
        for (const T & elem : elements)
        {
            *out = elem;
            ++out;
        }
    }

    void add(const T & x)
    {
        elements.insert(x);
    }

private:
    std::set<T> elements;
};

namespace storage
{
template <>
struct node_traits<std::unique_ptr<any_node_t<int> > >
{
    using serialized_t = std::unique_ptr<any_node_t<int> >;

    static serialized_t serialize(const std::unique_ptr<any_node_t<int> > & node)
    {

    }

    static std::unique_ptr<any_node_t<int> > deserialize(const serialized_t & serialized)
    {

    }
};
}

struct buffer_tree_t
{
    void add(std::int64_t x);

    template <typename OutIter>
    OutIter fetch_block(OutIter out)
    {
        leaf_t<node_id, std::int64_t> leaf = pop_left();
        leaf.write_elements(out);
        return out;
    }

    using node_id = int;
    using storage_node_t = std::unique_ptr<any_node_t<int> >;
    using buffer_storage_t = storage::memory<storage_node_t>;

private:
    leaf_t<node_id, std::int64_t> pop_left();

    node_id root_;
    buffer_storage_t storage;
};
