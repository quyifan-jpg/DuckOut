#pragma once

#include <google/protobuf/service.h>
#include <memory>
#include <string>
#include <unordered_map>
#include "epollServer.h" // 替换muduo头文件
#include "zookeeperutil.h"

// 服务提供者类，用于注册服务对象并提供RPC服务
class KrpcProvider
{
public:
    // 注册RPC服务对象
    void NotifyService(google::protobuf::Service *service);

    // 启动RPC服务节点
    void Run();

    // 析构函数
    ~KrpcProvider();

private:
    // 服务方法信息结构体
    struct ServiceInfo
    {
        google::protobuf::Service *service;                                                     // 服务对象指针
        std::unordered_map<std::string, const google::protobuf::MethodDescriptor *> method_map; // 方法名到方法描述符的映射
    };

    // 服务名到服务信息的映射表
    std::unordered_map<std::string, ServiceInfo> service_map;

    // 处理客户端连接的回调函数
    void OnConnection(std::shared_ptr<TcpConnection> conn);

    // 处理客户端消息的回调函数
    void OnMessage(std::shared_ptr<TcpConnection> conn, std::shared_ptr<Buffer> buffer);

    // 发送RPC响应的回调函数
    void SendRpcResponse(std::shared_ptr<TcpConnection> conn, google::protobuf::Message *response);
};
