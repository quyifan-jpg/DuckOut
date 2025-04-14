
#!/bin/bash
cd ..
set-x
echo "开始安装RPC项目所需依赖..."

#安装Cmake 
#sudo apt-get install cmake
# 更新包列表
echo "更新包列表..."
sudo apt-get update

# 安装基础工具
echo "安装基础工具..."
sudo apt-get install -y wget cmake build-essential unzip

# 安装zookeeper
echo "安装zookeeper..."
sudo apt-get install -y zookeeperd libzookeeper-mt-dev

# 安装protobuf
echo "安装protobuf..."
sudo apt-get install -y protobuf-compiler libprotobuf-dev

# 安装glog
echo "安装glog..."
sudo apt-get install -y libgoogle-glog-dev libgflags-dev

#安装boost库
sudo apt-get install -y libboost-all-dev


# 下载并安装muduo
echo "下载并安装muduo库..."
cd ~
wget https://www.programmercarl.com/download/muduo-master_2.zip
unzip muduo-master_2.zip


# 运行 Muduo 的 build.sh 脚本
echo "运行 build.sh 构建 Muduo..."
cd ~/muduo-master
chmod +x build.sh
sudo ./build.sh

# 进行 muduo 库安装
echo "进行 muduo 库安装... ./build.sh install"
sudo ./build.sh install

# 将头文件和库文件移动到标准系统目录
echo "将头文件和库文件移动到标准目录..."

if [ -d "/usr/include/muduo" ]; then
    echo "目标目录已存在，正在删除旧目录..."
    sudo rm -rf /usr/include/muduo
fi
sudo mv ~/build/release-install-cpp11/include/muduo /usr/include/

# 将库文件移动到 /usr/local/lib
sudo mv ~/build/release-install-cpp11/lib/* /usr/local/lib/
#如果说这里移动的位置有问题你可以从home/你的用户名/build/release-install-cpp11/lib/

echo "Muduo 安装完成！"
echo  "删除多余muduozip包"
cd ~ && sudo rm -f muduo-master_2.zip muduo-master_2.zip.*



