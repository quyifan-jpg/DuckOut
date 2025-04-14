#ifndef _Drpcchannel_H
#define _Drpcchannel_H

#include <string>

// DrpcChannel类，负责RPC调用的通信通道
class DrpcChannel {
public:
    // 发送消息
    virtual void SendMessage(const std::string& message) = 0;

    // 接收消息
    virtual std::string ReceiveMessage() = 0;

    // 虚析构函数
    virtual ~DrpcChannel() {}
};

#endif