cmake_minimum_required(VERSION 3.10)
project(DuckDBExample)

set(CMAKE_CXX_STANDARD 17)

# Include DuckDB headers
include_directories(include)

# Add the DuckDB library
add_executable(DuckDBExample src/main.cpp)
target_link_libraries(DuckDBExample PRIVATE duckdb)