#include "DrpcApplication.h"
#include "DrpcConfig.h"
#include <iostream>
#include <unistd.h>
DrpcApplication* DrpcApplication::m_application = nullptr;
std::mutex DrpcApplication::m_mutex;
DrpcConfig DrpcApplication::m_config;

void DrpcApplication::Init(int argc, char **argv) {
        if (argc < 2) {  // 如果命令行参数少于2个，说明没有指定配置文件
        std::cout << "格式: command -i <配置文件路径>" << std::endl;
        exit(EXIT_FAILURE);  // 退出程序
    }

    int o;
    std::string config_file;
    // 使用getopt解析命令行参数，-i表示指定配置文件
    while (-1 != (o = getopt(argc, argv, "i:"))) {
        switch (o) {
            case 'i':  // 如果参数是-i，后面的值就是配置文件的路径
                config_file = optarg;  // 将配置文件路径保存到config_file
                break;
            case '?':  // 如果出现未知参数（不是-i），提示正确格式并退出
                std::cout << "格式: command -i <配置文件路径>" << std::endl;
                exit(EXIT_FAILURE);
                break;
            case ':':  // 如果-i后面没有跟参数，提示正确格式并退出
                std::cout << "格式: command -i <配置文件路径>" << std::endl;
                exit(EXIT_FAILURE);
                break;
            default:
                break;
        }
    }

    // 加载配置文件
    m_config.LoadFromFile(config_file.c_str());
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