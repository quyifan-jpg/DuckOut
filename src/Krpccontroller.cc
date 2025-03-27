#include "Krpccontroller.h"

// 构造函数，初始化控制器状态
Krpccontroller::Krpccontroller()
{
    m_failed = false;           // 初始状态为未失败
    m_errText = "";             // 错误信息初始为空
    m_isCanceled = false;       // 初始化取消状态
    m_cancelCallback = nullptr; // 初始化取消回调为空
}

// 重置控制器状态，将失败标志和错误信息清空
void Krpccontroller::Reset()
{
    m_failed = false;           // 重置失败标志
    m_errText = "";             // 清空错误信息
    m_isCanceled = false;       // 重置取消状态
    m_cancelCallback = nullptr; // 重置取消回调
}

// 判断当前RPC调用是否失败
bool Krpccontroller::Failed() const
{
    return m_failed; // 返回失败标志
}

// 获取错误信息
std::string Krpccontroller::ErrorText() const
{
    return m_errText; // 返回错误信息
}

// 设置RPC调用失败，并记录失败原因
void Krpccontroller::SetFailed(const std::string &reason)
{
    m_failed = true;    // 设置失败标志
    m_errText = reason; // 记录失败原因
}

// 开始取消RPC调用
void Krpccontroller::StartCancel()
{
    m_isCanceled = true; // 设置取消标志
    if (m_cancelCallback != nullptr)
    {
        m_cancelCallback->Run(); // 如果有回调函数，执行它
    }
}

// 判断RPC调用是否被取消
bool Krpccontroller::IsCanceled() const
{
    return m_isCanceled; // 返回取消状态
}

// 注册取消回调函数
void Krpccontroller::NotifyOnCancel(google::protobuf::Closure *callback)
{
    m_cancelCallback = callback; // 保存取消回调函数
}