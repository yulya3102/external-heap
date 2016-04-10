project(external-heap)
cmake_minimum_required(VERSION 3.1)

set(CMAKE_CXX_FLAGS "-std=c++1y -Wall ${CMAKE_CXX_FLAGS}")
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

find_package(Boost REQUIRED)
find_package(GTest REQUIRED)

add_subdirectory(storage)
add_subdirectory(btree)
add_subdirectory(heap)
