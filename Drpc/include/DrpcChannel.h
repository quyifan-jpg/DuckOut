#ifndef _DRPC_CHANNEL_H_
#define _DRPC_CHANNEL_H_

#include <string>
#include <memory>
#include <functional>
#include "DrpcService.h"

namespace drpc
{

    class Channel
    {
    public:
        virtual ~Channel() = default;

        // 异步RPC调用
        virtual void CallMethod(
            const MethodDescriptor *method,
            DrpcController *controller,
            const Message *request,
            Message *response,
            std::function<void(bool)> done) = 0;

        // 同步RPC调用
        virtual bool CallMethodSync(
            const MethodDescriptor *method,
            DrpcController *controller,
            const Message *request,
            Message *response) = 0;

        // 连接到服务器
        virtual bool Connect(const std::string &host, int port) = 0;

        // 关闭连接
        virtual void Close() = 0;
    };

    // 具体的TCP通道实现
    class TcpChannel : public Channel
    {
    public:
        TcpChannel();
        ~TcpChannel() override;

        void CallMethod(
            const MethodDescriptor *method,
            DrpcController *controller,
            const Message *request,
            Message *response,
            std::function<void(bool)> done) override;

        bool CallMethodSync(
            const MethodDescriptor *method,
            DrpcController *controller,
            const Message *request,
            Message *response) override;

        bool Connect(const std::string &host, int port) override;
        void Close() override;

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };

} // namespace drpc

#endif // _DRPC_CHANNEL_H_