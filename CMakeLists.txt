project(external-heap)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_FLAGS "-std=c++11 -Wall ${CMAKE_CXX_FLAGS}")

set(HEAP
    external_heap.h external_heap.cpp
    buffer_tree.h   buffer_tree.cpp
)

add_library(external_heap SHARED ${HEAP})

find_package(GTest)
set(TESTS main.cpp)
add_executable(test_heap ${TESTS})
target_link_libraries(test_heap ${GTEST_LIBRARY} external_heap)
