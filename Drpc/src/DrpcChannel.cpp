#include "DrpcChannel.h"
#include "DrpcController.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <iostream>

namespace drpc
{

    class TcpChannel::Impl
    {
    public:
        Impl() : m_sockfd(-1) {}
        ~Impl() { Close(); }

        bool Connect(const std::string &host, int port)
        {
            if (m_sockfd != -1)
            {
                Close();
            }

            m_sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (m_sockfd == -1)
            {
                return false;
            }

            struct sockaddr_in server_addr;
            memset(&server_addr, 0, sizeof(server_addr));
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(port);
            if (inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr) <= 0)
            {
                Close();
                return false;
            }

            if (connect(m_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
            {
                Close();
                return false;
            }

            return true;
        }

        void Close()
        {
            if (m_sockfd != -1)
            {
                close(m_sockfd);
                m_sockfd = -1;
            }
        }

        bool SendRequest(const std::string &data, DrpcController *controller)
        {
            if (m_sockfd == -1)
            {
                controller->SetFailed("Connection not established");
                return false;
            }

            size_t total_sent = 0;
            while (total_sent < data.size())
            {
                if (controller->IsCanceled())
                {
                    controller->SetFailed("RPC call was cancelled");
                    return false;
                }

                ssize_t sent = send(m_sockfd, data.c_str() + total_sent,
                                    data.size() - total_sent, 0);
                if (sent <= 0)
                {
                    if (errno == EINTR)
                        continue;
                    controller->SetFailed(strerror(errno));
                    return false;
                }
                total_sent += sent;
            }
            return true;
        }

        bool ReceiveResponse(std::string *response, DrpcController *controller)
        {
            if (m_sockfd == -1)
            {
                controller->SetFailed("Connection not established");
                return false;
            }

            char buffer[4096];
            std::string received_data;

            while (true)
            {
                if (controller->IsCanceled())
                {
                    controller->SetFailed("RPC call was cancelled");
                    return false;
                }

                ssize_t bytes_read = recv(m_sockfd, buffer, sizeof(buffer), 0);
                if (bytes_read < 0)
                {
                    if (errno == EINTR)
                        continue;
                    controller->SetFailed(strerror(errno));
                    return false;
                }
                if (bytes_read == 0)
                {
                    break; // Connection closed by peer
                }
                received_data.append(buffer, bytes_read);
            }

            *response = std::move(received_data);
            return true;
        }

    private:
        int m_sockfd;
    };

    TcpChannel::TcpChannel() : impl_(new Impl()) {}
    TcpChannel::~TcpChannel() = default;

    void TcpChannel::CallMethod(
        const MethodDescriptor *method,
        DrpcController *controller,
        const Message *request,
        Message *response,
        std::function<void(bool)> done)
    {

        // 重置controller状态
        controller->Reset();

        // 设置取消回调
        controller->SetCancelCallback([this]()
                                      { impl_->Close(); });

        // 序列化请求
        std::string request_data;
        if (!request->SerializeToString(&request_data))
        {
            controller->SetFailed("Failed to serialize request");
            done(false);
            return;
        }

        // 发送请求
        if (!impl_->SendRequest(request_data, controller))
        {
            done(false);
            return;
        }

        // 接收响应
        std::string response_data;
        if (!impl_->ReceiveResponse(&response_data, controller))
        {
            done(false);
            return;
        }

        // 反序列化响应
        if (!response->ParseFromString(response_data))
        {
            controller->SetFailed("Failed to parse response");
            done(false);
            return;
        }

        done(true);
    }

    bool TcpChannel::CallMethodSync(
        const MethodDescriptor *method,
        DrpcController *controller,
        const Message *request,
        Message *response)
    {

        bool success = false;
        CallMethod(method, controller, request, response,
                   [&success](bool ok)
                   { success = ok; });
        return success;
    }

    bool TcpChannel::Connect(const std::string &host, int port)
    {
        return impl_->Connect(host, port);
    }

    void TcpChannel::Close()
    {
        impl_->Close();
    }

} // namespace drpc