#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ucp/api/ucp.h>
#include <ucs/config/types.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 13337
#define BUF_SIZE 1024

typedef struct {
    ucp_context_h context;
    ucp_worker_h worker;
    ucp_listener_h listener;
} server_context_t;

// 连接请求处理回调
void connection_request_handler(ucp_conn_request_h conn_request, void *arg)
{
    server_context_t *server_ctx = (server_context_t *)arg;
    ucp_ep_params_t ep_params;
    memset(&ep_params, 0, sizeof(ep_params));
    ep_params.field_mask = UCP_EP_PARAM_FIELD_CONN_REQUEST;
    ep_params.conn_request = conn_request;

    // 使用三参数接口创建 endpoint
    ucp_ep_h ep;
    ucs_status_t status = ucp_ep_create(server_ctx->worker, &ep_params, &ep);
    if (status != UCS_OK) {
        fprintf(stderr, "Failed to create endpoint: %s\n", ucs_status_string(status));
        return;
    }
    printf("Client connected.\n");
    // 后续可以在此处调用 ucp_tag_recv_nb 等接口进行数据收发
}

int main(int argc, char **argv)
{
    ucp_params_t ucp_params;
    ucp_config_t *config;
    ucs_status_t status;
    server_context_t server_ctx;
    memset(&server_ctx, 0, sizeof(server_context_t));

    // 初始化 UCX 配置
    status = ucp_config_read(NULL, NULL, &config);
    if (status != UCS_OK) {
        fprintf(stderr, "ucp_config_read failed\n");
        exit(1);
    }
    memset(&ucp_params, 0, sizeof(ucp_params));
    ucp_params.field_mask = UCP_PARAM_FIELD_FEATURES;
    ucp_params.features   = UCP_FEATURE_TAG; // 使用 tag 接口
    status = ucp_init(&ucp_params, config, &server_ctx.context);
    ucp_config_release(config);
    if (status != UCS_OK) {
        fprintf(stderr, "ucp_init failed\n");
        exit(1);
    }

    // 创建 worker
    ucp_worker_params_t worker_params;
    memset(&worker_params, 0, sizeof(worker_params));
    worker_params.field_mask = UCP_WORKER_PARAM_FIELD_THREAD_MODE;
    worker_params.thread_mode = UCS_THREAD_MODE_SINGLE;
    status = ucp_worker_create(server_ctx.context, &worker_params, &server_ctx.worker);
    if (status != UCS_OK) {
        fprintf(stderr, "ucp_worker_create failed\n");
        exit(1);
    }

    // 获取 worker 地址（可选，用于调试或地址发布）
    ucp_address_t *local_addr;
    size_t address_length;
    status = ucp_worker_get_address(server_ctx.worker, &local_addr, &address_length);
    if (status != UCS_OK) {
        fprintf(stderr, "ucp_worker_get_address failed\n");
        exit(1);
    }

    // 设置监听 socket 地址
    struct sockaddr_in listen_addr;
    memset(&listen_addr, 0, sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = INADDR_ANY;  // 或者明确指定IP: inet_pton(AF_INET, "192.168.100.1", &listen_addr.sin_addr);
    listen_addr.sin_port = htons(PORT);

    ucp_listener_params_t listener_params;
    memset(&listener_params, 0, sizeof(listener_params));
    listener_params.field_mask = UCP_LISTENER_PARAM_FIELD_SOCK_ADDR | UCP_LISTENER_PARAM_FIELD_CONN_HANDLER;
    listener_params.sockaddr.addr = (struct sockaddr *)&listen_addr;
    listener_params.sockaddr.addrlen = sizeof(listen_addr);
    listener_params.conn_handler.cb = connection_request_handler;
    listener_params.conn_handler.arg = &server_ctx;

    status = ucp_listener_create(server_ctx.worker, &listener_params, &server_ctx.listener);
    if (status != UCS_OK) {
        fprintf(stderr, "ucp_listener_create failed: %s\n", ucs_status_string(status));
        exit(1);
    }

    printf("Server listening on port %d...\n", PORT);

    // 进入 progress 循环等待连接和通信事件
    while (1) {
        ucp_worker_progress(server_ctx.worker);
        usleep(1000);
    }

    // 清理资源
    ucp_listener_destroy(server_ctx.listener);
    ucp_worker_release_address(server_ctx.worker, local_addr);
    ucp_worker_destroy(server_ctx.worker);
    ucp_cleanup(server_ctx.context);

    return 0;
}
