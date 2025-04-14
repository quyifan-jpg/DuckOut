cmake_minimum_required(VERSION 3.0)
project(Drpc)

set(CMAKE_CXX_STANDARD 11)

include_directories(include)

file(GLOB SOURCES
    src/*.cpp
)

add_library(drpc ${SOURCES})

add_subdirectory(drpc_example/caller)
add_subdirectory(drpc_example/callee)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set_target_properties(dclient dserver PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)