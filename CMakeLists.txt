project(external-heap)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_FLAGS "-std=c++1y -Wall ${CMAKE_CXX_FLAGS}")
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

find_package(Boost REQUIRED)
find_package(GTest REQUIRED)

add_subdirectory(storage)
add_subdirectory(btree)

set(BTREE_SRC
    btree/btree_data.h
    btree/btree.h
)

add_executable(test_btree
    ${BTREE_SRC}
    btree/btree_tests.cpp
)

target_link_libraries(test_btree btree_serialize storage ${GTEST_LIBRARY})

set(HEAP_SRC
    external_heap.h
)

add_executable(test_heap ${HEAP_SRC} ${BTREE_SRC} storage main.cpp)

target_link_libraries(test_heap btree_serialize ${GTEST_LIBRARY})
