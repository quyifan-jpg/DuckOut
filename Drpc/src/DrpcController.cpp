#include "DrpcController.h"
#include <iostream> // Added for logging

namespace drpc
{

    DrpcController::DrpcController()
        : m_failed(false), m_errorText(""), m_isCanceled(false), m_cancelCallback(nullptr)
    {
        // Constructor implementation
        std::cout << "DrpcController constructed" << std::endl;
    }

    DrpcController::~DrpcController()
    {
        // Destructor implementation
        std::cout << "DrpcController destructed" << std::endl;
    }

    void DrpcController::Invoke(const std::string &procedure, const std::string &request, std::string &response)
    {
        // Logic to invoke the remote procedure and handle the response
        std::cout << "Invoking procedure: " << procedure << std::endl;
        std::cout << "Request: " << request << std::endl;
        response = "Response from " + procedure; // Dummy response
    }

    void DrpcController::HandleResponse(const std::string &response)
    {
        // Logic to process the response from the remote procedure
        std::cout << "Handling response: " << response << std::endl;
    }

    void DrpcController::Reset()
    {
        m_failed = false;
        m_errorText = "";
        m_isCanceled = false;
        m_cancelCallback = nullptr;
    }

    bool DrpcController::Failed() const
    {
        return m_failed;
    }

    std::string DrpcController::ErrorText() const
    {
        return m_errorText;
    }

    void DrpcController::SetFailed(const std::string &reason)
    {
        m_failed = true;
        m_errorText = reason;
    }

    void DrpcController::StartCancel()
    {
        m_isCanceled = true;
        if (m_cancelCallback)
        {
            m_cancelCallback();
        }
    }

    bool DrpcController::IsCanceled() const
    {
        return m_isCanceled;
    }

    void DrpcController::SetCancelCallback(std::function<void()> callback)
    {
        m_cancelCallback = std::move(callback);
    }

} // namespace drpc