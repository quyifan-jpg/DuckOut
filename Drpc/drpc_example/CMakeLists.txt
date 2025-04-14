cmake_minimum_required(VERSION 3.0)
project(DrpcExample)

set(CMAKE_CXX_STANDARD 11)

# Add subdirectory for the callee (server)
add_subdirectory(callee)

# Add subdirectory for the caller (client)
add_subdirectory(caller)