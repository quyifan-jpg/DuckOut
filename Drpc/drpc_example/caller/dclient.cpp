#include "Drpcapplication.h"
#include "DrpcChannel.h"
#include "DrpcController.h"
#include "../example_service.h"
#include <iostream>

int main(int argc, char **argv)
{
    // 初始化DrpcApplication
    DrpcApplication::Init(argc, argv);

    // 创建Channel
    drpc::TcpChannel channel;
    if (!channel.Connect("localhost", 8888))
    {
        std::cerr << "Failed to connect to server" << std::endl;
        return 1;
    }

    // 创建Controller
    drpc::DrpcController controller;

    // 准备请求
    example::HelloRequest request;
    request.name = "World";

    // 准备接收响应
    example::HelloResponse response;

    // 获取方法描述符
    example::GreeterService service;
    auto method = service.GetMethodDescriptor("SayHello");

    // 发起同步调用
    bool success = channel.CallMethodSync(
        method,
        &controller,
        &request,
        &response);

    if (success)
    {
        std::cout << "Response: " << response.greeting << std::endl;
    }
    else
    {
        std::cerr << "RPC failed" << std::endl;
    }

    // 清理
    channel.Close();
    DrpcApplication::deleteInstance();
    return 0;
}
