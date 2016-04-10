add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/btree.pb.cc ${CMAKE_CURRENT_SOURCE_DIR}/btree.pb.h
    COMMAND protoc -I ${CMAKE_CURRENT_SOURCE_DIR} --cpp_out=${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/btree.proto
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/btree.proto
)

add_library(btree_proto btree.pb.cc)
target_link_libraries(btree_proto protobuf)
