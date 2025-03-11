#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ucp/api/ucp.h>
#include <ucs/config/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define SERVER_PORT 13337
#define SERVER_IP "192.168.100.1"
#define BUF_SIZE 1024
#define TEST_ITERATIONS 100  // 测试次数

// 在文件开头添加回调函数的声明
static void request_completed(void *request, ucs_status_t status, void *user_data) {
    // 回调函数不做任何事情，让主循环处理请求完成
}

// 获取当前时间（微秒）
static uint64_t get_current_time_us() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

static int send_message(ucp_worker_h worker, ucp_ep_h ep, const void *buffer, size_t length) {
    ucp_request_param_t send_param;
    memset(&send_param, 0, sizeof(send_param));
    send_param.op_attr_mask = UCP_OP_ATTR_FIELD_CALLBACK | 
                             UCP_OP_ATTR_FIELD_DATATYPE;
    send_param.datatype = ucp_dt_make_contig(1);
    send_param.cb.send = request_completed;

    // 发送消息
    ucs_status_ptr_t request = ucp_tag_send_nbx(ep, buffer, length, 0x1337, &send_param);
    
    if (UCS_PTR_IS_ERR(request)) {
        fprintf(stderr, "发送失败: %s\n", ucs_status_string(UCS_PTR_STATUS(request)));
        return -1;
    }

    // 等待发送完成
    if (UCS_PTR_IS_PTR(request)) {
        while (!ucp_request_is_completed(request)) {
            ucp_worker_progress(worker);  // 现在可以使用传入的 worker
        }
        ucp_request_free(request);
    }

    return length;
}

int main(int argc, char **argv)
{
    ucp_params_t ucp_params;
    ucp_config_t *config;
    ucs_status_t status;
    ucp_context_h context;
    ucp_worker_h worker;

    /* 初始化 UCX 配置 */
    status = ucp_config_read(NULL, NULL, &config);
    if (status != UCS_OK) {
        fprintf(stderr, "ucp_config_read failed\n");
        exit(1);
    }
    memset(&ucp_params, 0, sizeof(ucp_params));
    ucp_params.field_mask = UCP_PARAM_FIELD_FEATURES;
    ucp_params.features   = UCP_FEATURE_TAG; // 使用 tag 接口
    status = ucp_init(&ucp_params, config, &context);
    ucp_config_release(config);
    if (status != UCS_OK) {
        fprintf(stderr, "ucp_init failed\n");
        exit(1);
    }

    /* 创建 worker */
    ucp_worker_params_t worker_params;
    memset(&worker_params, 0, sizeof(worker_params));
    worker_params.field_mask = UCP_WORKER_PARAM_FIELD_THREAD_MODE;
    worker_params.thread_mode = UCS_THREAD_MODE_SINGLE;
    status = ucp_worker_create(context, &worker_params, &worker);
    if (status != UCS_OK) {
        fprintf(stderr, "ucp_worker_create failed\n");
        exit(1);
    }

    /* 构造服务器 socket 地址 */
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    /* 打印连接信息进行调试 */
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &server_addr.sin_addr, ip_str, INET_ADDRSTRLEN);
    printf("Attempting to connect to %s:%d\n", ip_str, ntohs(server_addr.sin_port));

    /* 使用 socket 地址方式创建 endpoint */
    ucp_ep_params_t ep_params;
    memset(&ep_params, 0, sizeof(ep_params));
    ep_params.field_mask = UCP_EP_PARAM_FIELD_FLAGS | UCP_EP_PARAM_FIELD_SOCK_ADDR;
    ep_params.flags = UCP_EP_PARAMS_FLAGS_CLIENT_SERVER;
    ep_params.sockaddr.addr = (const struct sockaddr*)&server_addr;
    ep_params.sockaddr.addrlen = sizeof(server_addr);

    ucp_ep_h ep;
    status = ucp_ep_create(worker, &ep_params, &ep);
    if (status != UCS_OK) {
        fprintf(stderr, "ucp_ep_create failed\n");
        exit(1);
    }
    printf("Connected to server %s:%d\n", SERVER_IP, SERVER_PORT);

    // 等待连接建立
    ucp_ep_params_t ep_params_local;
    memset(&ep_params_local, 0, sizeof(ep_params_local));
    ep_params_local.field_mask = UCP_EP_PARAM_FIELD_ERR_HANDLING_MODE;
    ep_params_local.err_mode = UCP_ERR_HANDLING_MODE_PEER;
    
    // 发送一个小消息来确认连接建立
    char test_msg[] = "test";
    if (send_message(worker, ep, test_msg, strlen(test_msg)) < 0) {
        fprintf(stderr, "连接测试失败\n");
        goto cleanup;
    }
    
    // 等待一小段时间确保连接完全建立
    usleep(1000000);  // 100ms

    /* 准备测试数据 */
    char send_buf[BUF_SIZE];
    memset(send_buf, 'A', BUF_SIZE);
    size_t msg_len = BUF_SIZE;

    // 开始测试
    uint64_t total_time = 0;
    uint64_t start_time, end_time;
    int successful_sends = 0;

    printf("Starting performance test: %d iterations of %zu bytes each\n", 
           TEST_ITERATIONS, msg_len);

    start_time = get_current_time_us();

    for (int i = 0; i < TEST_ITERATIONS; i++) {
        if (send_message(worker, ep, send_buf, msg_len) < 0) {
            fprintf(stderr, "Iteration %d failed\n", i);
            continue;
        }

        successful_sends++;
        
        // 每10次传输打印一次进度
        if ((i + 1) % 10 == 0) {
            printf("Completed %d/%d iterations\n", i + 1, TEST_ITERATIONS);
        }
    }

    end_time = get_current_time_us();
    total_time = end_time - start_time;

    // 计算并打印性能统计
    double total_time_sec = total_time / 1000000.0;
    double throughput = (successful_sends * msg_len) / (1024.0 * 1024.0 * 1024.0) / total_time_sec; // GB/s
    double latency = total_time / (double)successful_sends; // 微秒

    printf("\nPerformance Results:\n");
    printf("Total time: %.3f seconds\n", total_time_sec);
    printf("Successful transfers: %d/%d\n", successful_sends, TEST_ITERATIONS);
    printf("Average latency: %.2f us\n", latency);
    printf("Throughput: %.2f GB/s\n", throughput);
    printf("Total data transferred: %.2f MB\n", 
           (successful_sends * msg_len) / (1024.0 * 1024.0));

    cleanup:
    /* 清理资源 */
    ucp_ep_destroy(ep);
    ucp_worker_destroy(worker);
    ucp_cleanup(context);

    return 0;
}
