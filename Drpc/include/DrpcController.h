#ifndef _Drpccontroller_H
#define _Drpccontroller_H

#include <string>
#include <memory>

// DrpcController类，负责管理RPC调用及其执行
class DrpcController
{
public:
    DrpcController();
    ~DrpcController();

    // 调用远程过程
    // bool CallRemoteProcedure(const std::string& procedureName, const std::string& request, std::string& response);
    void Invoke(const std::string& procedure, const std::string& request, std::string& response);
    void HandleResponse(const std::string& response);

private:
    // 其他私有成员和方法
};

#endif