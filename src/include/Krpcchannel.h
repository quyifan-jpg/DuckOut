#ifndef _Krpcchannel_h_
#define _Krpcchannel_h_
// 此类是继承自google::protobuf::RpcChannel
// 目的是为了给客户端进行方法调用的时候，统一接收的
#include <google/protobuf/service.h>
#include "zookeeperutil.h"
#include <sys/types.h>
#include <string>
#include <mutex>
#include <atomic>
class KrpcChannel : public google::protobuf::RpcChannel
{
public:
    KrpcChannel(bool connectNow);
    virtual ~KrpcChannel()
    {
    }
    void CallMethod(const ::google::protobuf::MethodDescriptor *method,
                    ::google::protobuf::RpcController *controller,
                    const ::google::protobuf::Message *request,
                    ::google::protobuf::Message *response,
                    ::google::protobuf::Closure *done) override; // override可以验证是否是虚函数
private:
    int m_clientfd; // 存放客户端套接字
    std::string service_name;
    std::string m_ip;
    uint16_t m_port;
    std::string method_name;
    int m_idx; // 用来划分服务器ip和port的下标
    bool newConnect(const char *ip, uint16_t port);
    std::string QueryServiceHost(ZkClient *zkclient, std::string service_name, std::string method_name, int &idx);

    // 超时控制相关的帮助函数
    int sendWithTimeout(int fd, const char *data, size_t len, int timeout_ms);
    int recvWithTimeout(int fd, char *buf, size_t len, int timeout_ms);
    int setSocketNonBlocking(int fd);
    static std::mutex s_load_balance_mutex;            // 保护轮询计数器的互斥锁
    static std::atomic<int> s_next_server_index;       // 下一个服务器索引（全局计数器）
    std::string SelectServerByRoundRobin(const std::vector<std::string>& server_list);  // 轮询选择服务器
};
#endif
