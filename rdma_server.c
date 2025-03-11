#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ucp/api/ucp.h>
#include <ucs/config/types.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 13337
#define BUF_SIZE 1024
#define MAX_CLIENTS 10

typedef struct {
    ucp_ep_h ep;
    char recv_buffer[BUF_SIZE];
    int active;
} client_connection_t;

typedef struct {
    ucp_context_h context;
    ucp_worker_h worker;
    ucp_listener_h listener;
    client_connection_t clients[MAX_CLIENTS];
    int num_clients;
} server_context_t;

// 回调函数声明
static void recv_callback(void *request, ucs_status_t status, const ucp_tag_recv_info_t *info, void *user_data) {
    size_t *size = (size_t *)user_data;
    *size = info->length;
}

// 接收消息函数
static void post_receive(ucp_worker_h worker, client_connection_t *client) {
    size_t size = 0;
    ucp_request_param_t recv_param;
    memset(&recv_param, 0, sizeof(recv_param));
    recv_param.op_attr_mask = UCP_OP_ATTR_FIELD_CALLBACK | 
                             UCP_OP_ATTR_FIELD_DATATYPE |
                             UCP_OP_ATTR_FIELD_USER_DATA;
    recv_param.datatype = ucp_dt_make_contig(1);
    recv_param.cb.recv = recv_callback;
    recv_param.user_data = &size;

    ucs_status_ptr_t request = ucp_tag_recv_nbx(worker,
                                               client->recv_buffer,
                                               BUF_SIZE,
                                               0x1337,
                                               0,
                                               &recv_param);

    if (UCS_PTR_IS_ERR(request)) {
        fprintf(stderr, "接收请求失败: %s\n", 
                ucs_status_string(UCS_PTR_STATUS(request)));
        client->active = 0;
        return;
    }

    if (UCS_PTR_IS_PTR(request)) {
        ucp_request_free(request);
    }
}

// 连接请求处理回调
void connection_request_handler(ucp_conn_request_h conn_request, void *arg) {
    server_context_t *server_ctx = (server_context_t *)arg;
    
    if (server_ctx->num_clients >= MAX_CLIENTS) {
        fprintf(stderr, "达到最大客户端连接数\n");
        return;
    }

    // 找到一个空闲的客户端槽位
    int client_idx = server_ctx->num_clients++;
    client_connection_t *client = &server_ctx->clients[client_idx];

    // 创建 endpoint
    ucp_ep_params_t ep_params;
    memset(&ep_params, 0, sizeof(ep_params));
    ep_params.field_mask = UCP_EP_PARAM_FIELD_CONN_REQUEST;
    ep_params.conn_request = conn_request;

    ucs_status_t status = ucp_ep_create(server_ctx->worker, &ep_params, &client->ep);
    if (status != UCS_OK) {
        fprintf(stderr, "Failed to create endpoint: %s\n", ucs_status_string(status));
        server_ctx->num_clients--;
        return;
    }

    client->active = 1;
    printf("Client %d connected.\n", client_idx);

    // 为新客户端发布接收请求
    post_receive(server_ctx->worker, client);
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

    // 创建主 worker（用于监听连接）
    ucp_worker_params_t worker_params;
    memset(&worker_params, 0, sizeof(worker_params));
    worker_params.field_mask = UCP_WORKER_PARAM_FIELD_THREAD_MODE;
    worker_params.thread_mode = UCS_THREAD_MODE_MULTI;  // 使用多线程模式
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

    // 主事件循环
    while (1) {
        ucp_worker_progress(server_ctx.worker);

        // 检查所有活跃的客户端连接
        for (int i = 0; i < server_ctx.num_clients; i++) {
            client_connection_t *client = &server_ctx.clients[i];
            if (client->active) {
                // 检查是否收到数据
                if (client->recv_buffer[0] != '\0') {
                    printf("从客户端 %d 收到消息: %s\n", i, client->recv_buffer);
                    memset(client->recv_buffer, 0, BUF_SIZE);
                    // 发布新的接收请求
                    post_receive(server_ctx.worker, client);
                }
            }
        }

        usleep(1000);  // 避免CPU占用过高
    }

    // 清理资源
    ucp_listener_destroy(server_ctx.listener);
    ucp_worker_release_address(server_ctx.worker, local_addr);
    ucp_worker_destroy(server_ctx.worker);
    ucp_cleanup(server_ctx.context);

    return 0;
}
