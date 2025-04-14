#ifndef _Drpcapplication_H
#define _Drpcapplication_H

#include "Drpcconfig.h"
#include "Drpcchannel.h"
#include "Drpccontroller.h"
#include <mutex>

// Drpc基础类，负责框架的一些初始化操作
class DrpcApplication
{
public:
    static void Init(int argc, char **argv);
    static DrpcApplication &GetInstance();
    static void deleteInstance();
    static DrpcConfig &GetConfig();

private:
    static DrpcConfig m_config;
    static DrpcApplication *m_application; // 全局唯一单例访问对象
    static std::mutex m_mutex;
    DrpcApplication() {}
    ~DrpcApplication() {}
    DrpcApplication(const DrpcApplication &) = delete;
    DrpcApplication(DrpcApplication &&) = delete;
};

#endif