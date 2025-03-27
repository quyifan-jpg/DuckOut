#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include "add_simple.grpc.pb.h"

using calculator::AddRequest;
using calculator::AddResponse;
using calculator::Calculator;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

class CalculatorClient
{
public:
    CalculatorClient(std::shared_ptr<Channel> channel)
        : stub_(Calculator::NewStub(channel)) {}

    int Add(int a, int b)
    {
        AddRequest request;
        request.set_a(a);
        request.set_b(b);

        AddResponse response;
        ClientContext context;

        Status status = stub_->Add(&context, request, &response);

        if (status.ok())
        {
            return response.result();
        }
        else
        {
            std::cout << "RPC failed: " << status.error_message() << std::endl;
            return -1;
        }
    }

private:
    std::unique_ptr<Calculator::Stub> stub_;
};

int main(int argc, char **argv)
{
    std::string target_str = "localhost:50051";
    CalculatorClient calculator(
        grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

    int a = 10;
    int b = 20;

    std::cout << "Sending request: " << a << " + " << b << std::endl;
    int response = calculator.Add(a, b);
    std::cout << "Result: " << response << std::endl;

    return 0;
}