project(external-heap)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_FLAGS "-std=c++1y -Wall ${CMAKE_CXX_FLAGS}")
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

find_package(Boost REQUIRED)
find_package(GTest REQUIRED)

add_subdirectory(storage)
add_subdirectory(btree)

set(HEAP_SRC
    external_heap.h
)

add_executable(test_heap ${HEAP_SRC} storage main.cpp)

target_link_libraries(test_heap btree ${GTEST_LIBRARY})
