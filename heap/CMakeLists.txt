add_library(heap INTERFACE)
target_sources(heap INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/heap.h
)
target_include_directories(heap INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_executable(test_heap
    heap_tests.cpp
)
target_link_libraries(test_heap storage btree heap ${GTEST_LIBRARY})
