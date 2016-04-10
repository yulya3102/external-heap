#pragma once

#include "basic_storage.h"

#include <utils/undefined.h>

#include <boost/filesystem.hpp>
#include <string>

namespace fs = boost::filesystem;

namespace storage
{
template <typename Serialized>
struct directory : basic_storage<Serialized>
{
    directory(const fs::path & storage_dir)
        : storage_dir(storage_dir)
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
        undefined;
    }

    virtual void delete_node(const node_id & id)
    {
        fs::remove(storage_dir / std::to_string(id));
    }

    virtual void write_node(const node_id & id, Serialized * node)
    {
        undefined;
    }

private:
    mutable storage::node_id max_id;
    fs::path storage_dir;
};
}
