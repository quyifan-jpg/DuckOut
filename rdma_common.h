#ifndef RDMA_COMMON_H
#define RDMA_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>

#include <ucp/api/ucp.h>
#include <ucs/type/status.h>

#define SERVER_PORT 13337
#define TAG         0x1337
#define MAX_MESSAGE 128

// Helper function to exit on error
static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

// Simple progress loop for UCX worker
static void progress_worker(ucp_worker_h worker) {
    while (ucp_worker_progress(worker) > 0);
}

// Request handling function
static void request_init(void *request) {
    if (request != NULL) {
        ((ucp_request_param_t *)request)->op_attr_mask = UCP_OP_ATTR_FIELD_CALLBACK;
    }
}

// Socket helper functions
static void send_all(int sock, const void *buf, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t ret = send(sock, (const char*)buf + sent, len - sent, 0);
        if (ret <= 0)
            die("send");
        sent += ret;
    }
}

static void recv_all(int sock, void *buf, size_t len) {
    size_t recvd = 0;
    while (recvd < len) {
        ssize_t ret = recv(sock, (char*)buf + recvd, len - recvd, 0);
        if (ret <= 0)
            die("recv");
        recvd += ret;
    }
}

#endif 