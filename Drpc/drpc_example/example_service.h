#ifndef _EXAMPLE_SERVICE_H_
#define _EXAMPLE_SERVICE_H_

#include "DrpcService.h"

namespace example {

// 请求消息
class HelloRequest : public drpc::Message {
public:
    std::string name;

    bool SerializeToString(std::string* output) const override {
        *output = name;
        return true;
    }

    bool ParseFromString(const std::string& input) override {
        name = input;
        return true;
    }
};

// 响应消息
class HelloResponse : public drpc::Message {
public:
    std::string greeting;

    bool SerializeToString(std::string* output) const override {
        *output = greeting;
        return true;
    }

    bool ParseFromString(const std::string& input) override {
        greeting = input;
        return true;
    }
};

// 服务接口定义
class GreeterService : public drpc::Service {
public:
    virtual void SayHello(const HelloRequest* request,
                         HelloResponse* response,
                         std::function<void(bool)> done) = 0;

    // 实现Service接口
    void CallMethod(const drpc::MethodDescriptor* method,
                   drpc::DrpcController* controller,
                   const drpc::Message* request,
                   drpc::Message* response,
                   std::function<void(bool)> done) override {
        if (method->method_name == "SayHello") {
            SayHello(
                static_cast<const HelloRequest*>(request),
                static_cast<HelloResponse*>(response),
                done);
        }
    }

    const drpc::MethodDescriptor* GetMethodDescriptor(
        const std::string& method_name) override {
        if (method_name == "SayHello") {
            static drpc::MethodDescriptor desc {
                "GreeterService",
                "SayHello",
                "HelloRequest",
                "HelloResponse"
            };
            return &desc;
        }
        return nullptr;
    }
};

// 具体服务实现
class GreeterServiceImpl : public GreeterService {
public:
    void SayHello(const HelloRequest* request,
                  HelloResponse* response,
                  std::function<void(bool)> done) override {
        response->greeting = "Hello, " + request->name + "!";
        done(true);
    }
};

} // namespace example

#endif // _EXAMPLE_SERVICE_H_ 