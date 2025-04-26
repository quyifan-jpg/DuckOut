#include "Krpcapplication.h"
#include "../user.pb.h"
#include "Krpccontroller.h"
#include <iostream>
#include <atomic>
#include <thread>
#include <chrono>
#include "KrpcLogger.h"

void send_add_request(int thread_id,
                      std::atomic<int>& success_count,
                      std::atomic<int>& fail_count, int a, int b) {
  // 用于 RPC 调用的 Stub
  Kuser::UserServiceRpc_Stub stub(new KrpcChannel(false));

  // 构造 AddRequest
  Kuser::AddRequest request;
  request.set_a(a);  // 你可以改成随机或不同值
  request.set_b(b);

  // 准备响应和控制器
  Kuser::AddResponse response;
  KrpcController controller;

  // 发起 RPC 调用
  stub.Add(&controller, &request, &response, nullptr);

  // 检查结果
  if (controller.Failed()) {
    ++fail_count;
  } else if (response.result().errcode() != 0) {
    ++fail_count;
  } else {
    // 可选地校验 sum
    if (response.sum() != 123 + 456) {
      ++fail_count;
    } else {
      ++success_count;
    }
  }
}

// 发送 RPC 请求的函数，模拟客户端调用远程服务
void send_request(int thread_id, std::atomic<int> &success_count, std::atomic<int> &fail_count) {
    // 创建一个 UserServiceRpc_Stub 对象，用于调用远程的 RPC 方法
    Kuser::UserServiceRpc_Stub stub(new KrpcChannel(false));

    // 设置 RPC 方法的请求参数
    Kuser::LoginRequest request;
    request.set_name("zhangsan");  // 设置用户名
    request.set_pwd("123456");    // 设置密码

    // 定义 RPC 方法的响应参数
    Kuser::LoginResponse response;
    KrpcController controller;  // 创建控制器对象，用于处理 RPC 调用过程中的错误

    // 调用远程的 Login 方法
    stub.Login(&controller, &request, &response, nullptr);

    // 检查 RPC 调用是否成功
    if (controller.Failed()) {  // 如果调用失败
        std::cout << controller.ErrorText() << std::endl;  // 打印错误信息
        fail_count++;  // 失败计数加 1
    } else {  // 如果调用成功
        if (0 == response.result().errcode()) {  // 检查响应中的错误码
            std::cout << "rpc login response success:" << response.success() << std::endl;  // 打印成功信息
            success_count++;  // 成功计数加 1
        } else {  // 如果响应中有错误
            std::cout << "rpc login response error : " << response.result().errmsg() << std::endl;  // 打印错误信息
            fail_count++;  // 失败计数加 1
        }
    }
}

int main(int argc, char** argv) {
  KrpcApplication::Init(argc, argv);
  KrpcLogger logger("AddBenchmark");

  const int thread_count = 1;        // 并发线程数
  const int requests_per_thread = 10; // 每线程请求数

  std::vector<std::thread> threads;
  std::atomic<int> success_count(0), fail_count(0);

  auto start = std::chrono::high_resolution_clock::now();

  // 启动线程
  for (int i = 0; i < thread_count; ++i) {
    threads.emplace_back([i, &success_count, &fail_count, requests_per_thread] {
      for (int j = 0; j < requests_per_thread; ++j) {
        send_add_request(i, success_count, fail_count, j, i*100000);
      }
    });
  }
  // 等待
  for (auto& t : threads) t.join();

  auto end = std::chrono::high_resolution_clock::now();
  double elapsed = std::chrono::duration<double>(end - start).count();
  int total = thread_count * requests_per_thread;

  LOG(INFO) << "Total requests: " << total;
  LOG(INFO) << "Success: "        << success_count;
  LOG(INFO) << "Failed: "         << fail_count;
  LOG(INFO) << "Elapsed(sec): "   << elapsed;
  LOG(INFO) << "QPS: "            << (total / elapsed);

  return 0;
}