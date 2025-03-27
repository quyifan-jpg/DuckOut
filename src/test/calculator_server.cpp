#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include "add_simple.grpc.pb.h"

using calculator::AddRequest;
using calculator::AddResponse;
using calculator::Calculator;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

// 实现 Calculator 服务
class CalculatorServiceImpl final : public Calculator::Service
{
    Status Add(ServerContext *context, const AddRequest *request,
               AddResponse *response) override
    {
        // 实现加法逻辑
        response->set_result(request->a() + request->b());
        std::cout << "Received request: " << request->a() << " + " << request->b()
                  << " = " << response->result() << std::endl;
        return Status::OK;
    }
};

void RunServer()
{
    std::string server_address("0.0.0.0:50051");
    CalculatorServiceImpl service;

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();
}

int main(int argc, char **argv)
{
    RunServer();
    return 0;
}