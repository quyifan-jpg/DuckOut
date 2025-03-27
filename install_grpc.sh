#!/bin/bash

# 安装必要的依赖
sudo apt-get update
sudo apt-get install -y build-essential autoconf libtool pkg-config cmake git

# 克隆grpc仓库
git clone --recurse-submodules -b v1.58.0 --depth 1 --shallow-submodules https://github.com/grpc/grpc

# 创建构建目录
cd grpc
mkdir -p cmake/build
cd cmake/build

# 配置和构建
cmake -DgRPC_INSTALL=ON \
      -DgRPC_BUILD_TESTS=OFF \
      -DCMAKE_INSTALL_PREFIX=/usr/local \
      ../..

# 编译
make -j$(nproc)

# 安装
sudo make install

# 更新动态链接器运行时绑定
sudo ldconfig

# 验证安装
which grpc_cpp_plugin 