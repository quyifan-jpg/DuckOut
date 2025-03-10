#include <stdio.h>
#include <pthread.h>
#include <ucp/api/ucp.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "config.h"

const ucp_tag_t TAG = 0x1337;

// 回调函数的上下文结构体
struct send_context {
    int sent_size;
    ucs_status_t status;
};

// 发送回调函数
static void send_callback(void *request, ucs_status_t status, void *user_data) {
    if (status != UCS_OK) {
        printf("Send callback failed: %s\n", ucs_status_string(status));
        return;
    }

    struct send_context *ctx = (struct send_context *)user_data;
    ctx->sent_size = (int)(uintptr_t)request;
    ctx->status = status;
}

int send_message(ucp_worker_h worker, ucp_ep_h ep, void *buffer, size_t msgSize) {
    struct send_context ctx = {0};

    ucp_request_param_t send_param;
    memset(&send_param, 0, sizeof(send_param));
    send_param.op_attr_mask = UCP_OP_ATTR_FIELD_CALLBACK | UCP_OP_ATTR_FIELD_USER_DATA;
    send_param.cb.send = send_callback;
    send_param.user_data = &ctx;

    void *request = ucp_tag_send_nbx(ep, buffer, msgSize, TAG, &send_param);
    if (UCS_PTR_IS_ERR(request)) {
        printf("Send request failed: %s\n", ucs_status_string(UCS_PTR_STATUS(request)));
        return -1;
    }

    if (UCS_PTR_IS_PTR(request)) {
        while (!ucp_request_is_completed(request)) {
            ucp_worker_progress(worker);
        }
        ucp_request_free(request);
    } else {
        ctx.sent_size = msgSize;
    }

    return ctx.sent_size;
}

// 接收回调上下文结构体
struct recv_context {
    int size;
    ucs_status_t status;
};

// 接收回调函数
static void recv_callback(void *request, ucs_status_t status, 
                        const ucp_tag_recv_info_t *info, void *user_data) {
    if (status != UCS_OK) {
        printf("Receive callback failed: %s\n", ucs_status_string(status));
        return;
    }

    struct recv_context *ctx = (struct recv_context *)user_data;
    ctx->size = info->length;
    ctx->status = status;
}

int receive_message(ucp_worker_h worker, ucp_ep_h ep, void *buffer, size_t msgSize) {
    struct recv_context ctx = {0};

    ucp_request_param_t recv_param;
    memset(&recv_param, 0, sizeof(recv_param));
    recv_param.op_attr_mask = UCP_OP_ATTR_FIELD_CALLBACK | UCP_OP_ATTR_FIELD_USER_DATA;
    recv_param.cb.recv = recv_callback;
    recv_param.user_data = &ctx;

    void *request = ucp_tag_recv_nbx(worker, buffer, msgSize, TAG, 0, &recv_param);

    if (UCS_PTR_IS_ERR(request)) {
        printf("Receive request failed: %s\n", ucs_status_string(UCS_PTR_STATUS(request)));
        return -1;
    }

    if (UCS_PTR_IS_PTR(request)) {
        while (!ucp_request_is_completed(request)) {
            ucp_worker_progress(worker);
        }
        ucp_request_free(request);
    } else {
        ctx.size = msgSize;
    }

    return ctx.size;
}

void* ThreadFunc(void* args) {
    // 解构传入的参数
    struct {
        ucp_worker_h worker;
        ucp_ep_h ep;
    } *thread_args = (typeof(thread_args))args;
    
    ucp_worker_h worker = thread_args->worker;
    ucp_ep_h ep = thread_args->ep;
    free(thread_args); // 释放参数结构体
    
    int iResult = 0;
    printf("Waiting for the first message...\n");

    // Receive the first message
    int msgSize = 0;
    iResult = receive_message(worker, ep, (char *)&msgSize, sizeof(int));
    if (iResult < 0 || iResult != sizeof(int)) {
        printf("Error receiving the first message\n");
        return NULL;
    }

    printf("Starting measurement (%d-byte messages)...\n", msgSize);

    char *recvBuffer = malloc(msgSize);
    if (!recvBuffer) {
        printf("Failed to allocate receive buffer\n");
        return NULL;
    }
    memset(recvBuffer, 0, msgSize);

    int oneReceive = 0;
    int remainingBytesToReceive = msgSize;

    int oneSend = 0;
    int remainingBytesToSend = msgSize;
    int actualBytesToSend;

    // Receive message from client
    iResult = receive_message(worker, ep, recvBuffer + oneReceive, remainingBytesToReceive);
    if (iResult < 0) {
        printf("Error receiving message\n");
        free(recvBuffer);
        return NULL;
    }

    // Send response back
    while (oneSend < msgSize) {
        actualBytesToSend = remainingBytesToSend > SEND_PACKET_SIZE ? 
                           SEND_PACKET_SIZE : remainingBytesToSend;
        
        iResult = send_message(worker, ep, recvBuffer + oneSend, actualBytesToSend);
        if (iResult < 0) {
            printf("Error sending message\n");
            free(recvBuffer);
            return NULL;
        }

        oneSend += iResult;
        remainingBytesToSend = msgSize - oneSend;
    }

    printf("Measurement completes\n");

    free(recvBuffer);
    return NULL;
}

static void accept_handler(ucp_ep_h ep, void *arg) {
    printf("Accepted connection request\n");
    ucp_worker_h worker = (ucp_worker_h)arg;

    ucs_status_t status = ucp_worker_flush(worker);
    if (status != UCS_OK) {
        printf("Failed to flush UCX worker: %s\n", ucs_status_string(status));
    }

    // 创建线程参数结构体
    struct {
        ucp_worker_h worker;
        ucp_ep_h ep;
    } *thread_args = malloc(sizeof(*thread_args));
    
    thread_args->worker = worker;
    thread_args->ep = ep;

    // 创建新线程
    pthread_t thread_id;
    int ret = pthread_create(&thread_id, NULL, ThreadFunc, thread_args);
    if (ret != 0) {
        printf("Failed to create thread: %d\n", ret);
        free(thread_args);
    }
    pthread_detach(thread_id); // 设置为分离状态
}

int main() {
    int iResult = 0;
    
    // UCX initialization
    ucp_context_h context;
    ucp_worker_h worker;
    ucp_config_t *config;

    // Initialize UCX parameters
    ucp_params_t ucp_params;
    memset(&ucp_params, 0, sizeof(ucp_params));
    ucp_params.field_mask = UCP_PARAM_FIELD_FEATURES;
    ucp_params.features = UCP_FEATURE_TAG | UCP_FEATURE_STREAM;

    if (ucp_config_read(NULL, NULL, &config) != UCS_OK) {
        printf("Failed to read UCX config\n");
        return 1;
    }

    if (ucp_init(&ucp_params, config, &context) != UCS_OK) {
        printf("Failed to initialize UCX context\n");
        ucp_config_release(config);
        return 1;
    }
    ucp_config_release(config);

    // Create worker
    ucp_worker_params_t worker_params;
    memset(&worker_params, 0, sizeof(worker_params));
    worker_params.field_mask = UCP_WORKER_PARAM_FIELD_THREAD_MODE;
    worker_params.thread_mode = UCS_THREAD_MODE_SINGLE;

    if (ucp_worker_create(context, &worker_params, &worker) != UCS_OK) {
        printf("Failed to create UCX worker\n");
        ucp_cleanup(context);
        return 1;
    }

    // Create UCX listener
    ucp_listener_params_t listener_params;
    memset(&listener_params, 0, sizeof(listener_params));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERVER_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    listener_params.field_mask = UCP_LISTENER_PARAM_FIELD_SOCK_ADDR |
                               UCP_LISTENER_PARAM_FIELD_ACCEPT_HANDLER;
    listener_params.sockaddr.addr = (struct sockaddr *)&addr;
    listener_params.sockaddr.addrlen = sizeof(addr);
    listener_params.accept_handler.cb = accept_handler;
    listener_params.accept_handler.arg = worker;

    ucp_listener_h listener;
    ucs_status_t status = ucp_listener_create(worker, &listener_params, &listener);
    if (status != UCS_OK) {
        printf("Failed to create UCX listener: %s\n", ucs_status_string(status));
        return -1;
    }

    printf("UCX listener is listening on port %d\n", SERVER_PORT);
    printf("Waiting for clients...\n");

    // Main loop
    while (1) {
        status = ucp_worker_progress(worker);
        if (status != UCS_OK) {
            printf("Failed to progress UCX worker: %s\n", ucs_status_string(status));
            break;
        }
    }

    // Cleanup
    ucp_listener_destroy(listener);
    ucp_worker_destroy(worker);
    ucp_cleanup(context);

    return 0;
}