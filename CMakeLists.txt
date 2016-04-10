project(external-heap)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_FLAGS "-std=c++11 -Wall ${CMAKE_CXX_FLAGS}")
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

find_package(Boost REQUIRED)
find_package(GTest REQUIRED)

set(STORAGE_SRC
    storage/memory.h
    storage/cache.h
)

set(BTREE_SRC
    btree/btree_data.h
    btree/btree.h
)

add_executable(test_btree
    ${BTREE_SRC} ${STORAGE_SRC}
    btree/btree_tests.cpp
)

target_link_libraries(test_btree ${GTEST_LIBRARY})

#set(HEAP
    #external_heap.h external_heap.cpp
    #buffer_tree.h   buffer_tree.cpp
    #utils/undefined.h
#)

#add_library(external_heap SHARED ${HEAP})

#set(TESTS main.cpp)
#add_executable(test_heap ${TESTS})
#target_link_libraries(test_heap ${GTEST_LIBRARY} external_heap ${Boost_LIBRARIES})
