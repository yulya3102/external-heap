add_executable(test_comparsion comparsion.cpp heap.h)

target_link_libraries(test_comparsion
    storage btree heap
    ${GTEST_LIBRARY}
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
)
