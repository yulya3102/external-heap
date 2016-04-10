add_subdirectory(serialize)

add_library(btree_serialize
    serialize.h serialize.cpp
)

target_link_libraries(btree_serialize btree_proto)
