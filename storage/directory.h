#pragma once

#include "basic_storage.h"

#include <utils/undefined.h>

#include <boost/filesystem.hpp>
#include <string>
#include <fstream>
#include <ios>

namespace fs = boost::filesystem;

namespace storage
{
template <typename Serialized>
struct directory : basic_storage<Serialized>
{
    directory(const fs::path & storage_dir)
        : max_id(0)
        , storage_dir(storage_dir)
    {
        fs::create_directory(storage_dir);

        fs::directory_iterator end;
        for (fs::directory_iterator it(storage_dir); it != end; ++it)
        {
            fs::path node_file = it->path().filename();
            storage::node_id parsed = std::stoi(node_file.string());
            if (parsed > max_id)
                max_id = parsed;
        }
    }

    virtual node_id new_node() const
    {
        return ++max_id;
    }

    virtual std::shared_ptr<Serialized> load_node(const node_id & id) const
    {
        fs::path node = node_path(id);
        std::ifstream in(node.string(), std::ios_base::binary);
        std::string * data = new std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
        return std::shared_ptr<Serialized>(data);
    }

    virtual void delete_node(const node_id & id)
    {
        fs::remove(node_path(id));
    }

    virtual void write_node(const node_id & id, Serialized * node)
    {
        std::ofstream out(node_path(id).string(), std::ios_base::binary);
        out << *node;
        out.close();
    }

    fs::path node_path(const node_id & id) const
    {
        return storage_dir / std::to_string(id);
    }

private:
    mutable storage::node_id max_id;
    fs::path storage_dir;
};
}
