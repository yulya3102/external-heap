add_library(storage INTERFACE)

target_sources(storage INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/basic_storage.h
    ${CMAKE_CURRENT_SOURCE_DIR}/memory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/cache.h
)

target_include_directories(storage INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)
