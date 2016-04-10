add_subdirectory(serialize)

add_library(btree_serialize
    serialize.h serialize.cpp
)
target_link_libraries(btree_serialize btree_proto)

add_library(btree INTERFACE)
target_sources(btree INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/btree_data.h
    ${CMAKE_CURRENT_SOURCE_DIR}/btree.h
)
target_link_libraries(btree INTERFACE btree_serialize)

add_executable(test_btree
    btree_tests.cpp
)
target_link_libraries(test_btree storage btree
    ${GTEST_LIBRARY}
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
)
