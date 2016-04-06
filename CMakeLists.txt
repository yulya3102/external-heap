project(external-heap)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_FLAGS "-std=c++11 -Wall ${CMAKE_CXX_FLAGS}")
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

find_package(Boost COMPONENTS filesystem system REQUIRED)

set(HEAP
    external_heap.h external_heap.cpp
    buffer_tree.h   buffer_tree.cpp
    storage/memory.h
    btree/btree.h
)

#add_library(external_heap SHARED ${HEAP})

find_package(GTest REQUIRED)

set(TESTS main.cpp)
#add_executable(test_heap ${TESTS})
#target_link_libraries(test_heap ${GTEST_LIBRARY} external_heap ${Boost_LIBRARIES})

add_executable(test_btree btree/btree.h btree/btree_tests.cpp storage/memory.h)
target_link_libraries(test_btree ${GTEST_LIBRARY})
