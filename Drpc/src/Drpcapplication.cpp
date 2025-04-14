#include "Drpcapplication.h"
#include "Drpcconfig.h"
#include <iostream>

DrpcApplication* DrpcApplication::m_application = nullptr;
std::mutex DrpcApplication::m_mutex;
DrpcConfig DrpcApplication::m_config;

void DrpcApplication::Init(int argc, char **argv) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_application == nullptr) {
        m_application = new DrpcApplication();
        // Initialization logic here
        std::cout << "DrpcApplication initialized." << std::endl;
    }
}

DrpcApplication& DrpcApplication::GetInstance() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_application == nullptr) {
        throw std::runtime_error("DrpcApplication not initialized.");
    }
    return *m_application;
}

void DrpcApplication::deleteInstance() {
    std::lock_guard<std::mutex> lock(m_mutex);
    delete m_application;
    m_application = nullptr;
}

DrpcConfig& DrpcApplication::GetConfig() {
    return m_config;
}