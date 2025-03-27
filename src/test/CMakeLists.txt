cmake_minimum_required(VERSION 3.10)
project(grpc_calculator)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Protobuf REQUIRED)

# 设置生成目录
set(GENERATED_PROTO_DIR "${CMAKE_CURRENT_BINARY_DIR}")
file(MAKE_DIRECTORY ${GENERATED_PROTO_DIR})

# 设置proto文件
set(PROTO_FILE "${CMAKE_CURRENT_SOURCE_DIR}/add_simple.proto")

# 生成protobuf和gRPC文件
add_custom_command(
    OUTPUT 
        "${GENERATED_PROTO_DIR}/add_simple.pb.cc"
        "${GENERATED_PROTO_DIR}/add_simple.pb.h"
        "${GENERATED_PROTO_DIR}/add_simple.grpc.pb.cc"
        "${GENERATED_PROTO_DIR}/add_simple.grpc.pb.h"
    COMMAND protoc
    ARGS 
        --cpp_out="${GENERATED_PROTO_DIR}"
        --grpc_out="${GENERATED_PROTO_DIR}"
        --plugin=protoc-gen-grpc=/usr/bin/grpc_cpp_plugin
        -I "${CMAKE_CURRENT_SOURCE_DIR}"
        "${PROTO_FILE}"
    DEPENDS "${PROTO_FILE}"
)

# 添加生成的文件
add_custom_target(generate_protos ALL
    DEPENDS
        "${GENERATED_PROTO_DIR}/add_simple.pb.cc"
        "${GENERATED_PROTO_DIR}/add_simple.pb.h"
        "${GENERATED_PROTO_DIR}/add_simple.grpc.pb.cc"
        "${GENERATED_PROTO_DIR}/add_simple.grpc.pb.h"
)

# Include directories
include_directories(${GENERATED_PROTO_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(/usr/local/include)  # gRPC 头文件目录

# 设置库的路径
link_directories(/usr/local/lib)

# Server executable
add_executable(calculator_server 
    calculator_server.cpp
    ${GENERATED_PROTO_DIR}/add_simple.pb.cc
    ${GENERATED_PROTO_DIR}/add_simple.grpc.pb.cc
)

add_dependencies(calculator_server generate_protos)

target_link_libraries(calculator_server
    ${PROTOBUF_LIBRARIES}
    grpc++
    grpc
    gpr
)

# Client executable
add_executable(calculator_client 
    calculator_client.cpp
    ${GENERATED_PROTO_DIR}/add_simple.pb.cc
    ${GENERATED_PROTO_DIR}/add_simple.grpc.pb.cc
)

add_dependencies(calculator_client generate_protos)

target_link_libraries(calculator_client
    ${PROTOBUF_LIBRARIES}
    grpc++
    grpc
    gpr
) 