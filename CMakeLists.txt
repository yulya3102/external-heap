project(external-heap)
cmake_minimum_required(VERSION 2.8)

set(HEAP
    external_heap.h external_heap.cpp
)

add_library(external_heap ${HEAP})

find_package(GTest)
set(TESTS main.cpp)
add_executable(test_heap ${TESTS})
target_link_libraries(test_heap ${GTEST_LIBRARY} external_heap)
