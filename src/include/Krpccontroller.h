#ifndef _Krpccontroller_H
#define _Krpccontroller_H

#include <google/protobuf/service.h>
#include <string>
// 用于描述RPC调用的控制器
// 其主要作用是跟踪RPC方法调用的状态、错误信息并提供控制功能(如取消调用)。
class KrpcController : public google::protobuf::RpcController
{
public:
    KrpcController();
    void Reset() override;
    bool Failed() const override;
    std::string ErrorText() const override;
    void SetFailed(const std::string &reason) override;

    // 客户端相关的控制方法
    void StartCancel() override;
    bool IsCanceled() const override;
    void NotifyOnCancel(google::protobuf::Closure *callback) override;

    // 新增：超时控制相关方法
    void SetTimeout(int timeout_ms); // 设置超时时间（毫秒）
    int GetTimeout() const;          // 获取超时时间
    bool IsTimedOut() const;         // 检查是否已超时
    void SetTimedOut();              // 设置为已超时状态

private:
    bool m_failed;         // 失败标志
    std::string m_errText; // 错误信息
    bool m_is_canceled;    // 取消标志
    bool m_is_timedout;    // 超时标志
    int m_timeout_ms;      // 超时时间（毫秒）
    google::protobuf::Closure* m_cancelCallback;  // 取消操作的回调函数

};

#endif