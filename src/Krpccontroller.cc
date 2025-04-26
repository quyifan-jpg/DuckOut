#include "Krpccontroller.h"

// 构造函数，初始化控制器状态
KrpcController::KrpcController()
    : m_failed(false), m_is_canceled(false), m_is_timedout(false), m_timeout_ms(5000) // 默认超时5秒
{
    m_errText = "";             // 错误信息初始为空
    m_cancelCallback = nullptr; // 初始化取消回调为空
}

// 重置控制器状态，将失败标志和错误信息清空
void KrpcController::Reset()
{
    m_failed = false;      // 重置失败标志
    m_errText = "";        // 清空错误信息
    m_is_canceled = false; // 重置取消状态
    m_is_timedout = false; // 重置超时状态
    // 不重置超时时间，保持用户设置的值
}

// 判断当前RPC调用是否失败
bool KrpcController::Failed() const
{
    return m_failed; // 返回失败标志
}

// 获取错误信息
std::string KrpcController::ErrorText() const
{
    return m_errText; // 返回错误信息
}

// 设置RPC调用失败，并记录失败原因
void KrpcController::SetFailed(const std::string &reason)
{
    m_failed = true;    // 设置失败标志
    m_errText = reason; // 记录失败原因
}

// 开始取消RPC调用
void KrpcController::StartCancel()
{
    m_is_canceled = true; // 设置取消标志
    if (m_cancelCallback != nullptr)
    {
        m_cancelCallback->Run(); // 如果有回调函数，执行它
    }
}

// 判断RPC调用是否被取消
bool KrpcController::IsCanceled() const
{
    return m_is_canceled; // 返回取消状态
}

// 注册取消回调函数
void KrpcController::NotifyOnCancel(google::protobuf::Closure *callback)
{
    m_cancelCallback = callback; // 保存取消回调函数
}

// 新增：超时控制相关方法实现
void KrpcController::SetTimeout(int timeout_ms)
{
    m_timeout_ms = timeout_ms;
}

int KrpcController::GetTimeout() const
{
    return m_timeout_ms;
}

bool KrpcController::IsTimedOut() const
{
    return m_is_timedout;
}

void KrpcController::SetTimedOut()
{
    m_is_timedout = true;
    SetFailed("RPC call timed out");
}