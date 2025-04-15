#include "Drpcapplication.h"
#include "DrpcServer.h"
#include "../example_service.h"
#include <iostream>

int main(int argc, char **argv)
{
    // Initialize DrpcApplication
    DrpcApplication::Init(argc, argv);

    // 创建服务器实例
    drpc::Server server;

    // 创建并注册Greeter服务
    auto service = std::make_unique<example::GreeterServiceImpl>();
    server.RegisterService(service.get());

    // 启动服务器
    if (!server.Start("0.0.0.0", 8888))
    {
        std::cerr << "Failed to start server" << std::endl;
        return 1;
    }

    std::cout << "Server started on port 8888" << std::endl;

    // 运行服务器（阻塞）
    server.Run();

    // Clean up DrpcApplication before exiting
    DrpcApplication::deleteInstance();
    return 0;
}
