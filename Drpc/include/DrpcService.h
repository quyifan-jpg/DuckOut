#ifndef _DRPC_SERVICE_H_
#define _DRPC_SERVICE_H_

#include <string>
#include <memory>
#include <functional>
#include "DrpcController.h"

namespace drpc
{

    // 抽象消息接口
    class Message
    {
    public:
        virtual ~Message() = default;
        virtual bool SerializeToString(std::string *output) const = 0;
        virtual bool ParseFromString(const std::string &input) = 0;
    };

    // RPC方法描述
    struct MethodDescriptor
    {
        std::string service_name;
        std::string method_name;
        std::string request_type;
        std::string response_type;
    };

    // 服务接口
    class Service
    {
    public:
        virtual ~Service() = default;

        // 调用方法
        virtual void CallMethod(
            const MethodDescriptor *method,
            DrpcController *controller,
            const Message *request,
            Message *response,
            std::function<void(bool)> done) = 0;

        // 获取方法描述
        virtual const MethodDescriptor *GetMethodDescriptor(const std::string &method_name) = 0;
    };

} // namespace drpc

#endif // _DRPC_SERVICE_H_