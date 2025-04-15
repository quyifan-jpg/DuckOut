#ifndef _DRPC_SERVER_H_ 
#define _DRPC_SERVER_H_

#include <string>
#include <memory>
#include <unordered_map>
#include "DrpcService.h"

namespace drpc {

class Server {
public:
    Server();
    ~Server();

    // 注册服务
    bool RegisterService(Service* service);

    // 启动服务器
    bool Start(const std::string& host, int port);

    // 运行服务器（阻塞）
    void Run();

    // 停止服务器
    void Stop();

    // 等待服务器停止
    void Wait();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// 服务注册器 - 用于自动注册服务
class ServiceRegistry {
public:
    static ServiceRegistry& Instance();
    
    void RegisterService(const std::string& service_name, std::unique_ptr<Service> service);
    Service* GetService(const std::string& service_name);

private:
    ServiceRegistry() = default;
    std::unordered_map<std::string, std::unique_ptr<Service>> services_;
};

} // namespace drpc

#endif // _DRPC_SERVER_H_ 