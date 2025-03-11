#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ucp/api/ucp.h>
#include <ucs/config/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>

// Server-specific definitions
#define SERVER_PORT 13337
#define BUF_SIZE 1024
#define MAX_CLIENTS 10

// Client connection structure
typedef struct {
    ucp_ep_h ep;
    char recv_buffer[BUF_SIZE];
    int active;
} client_connection_t;

// Server context structure
typedef struct {
    ucp_context_h context;
    ucp_worker_h worker;
    ucp_listener_h listener;
    client_connection_t clients[MAX_CLIENTS];
    int num_clients;
} server_context_t;

// Common utility functions
void request_completed(void *request, ucs_status_t status, void *user_data);
uint64_t get_current_time_us();

// UCX initialization and cleanup functions
ucs_status_t init_ucp_context(ucp_context_h *context);
ucs_status_t init_ucp_worker(ucp_context_h context, ucp_worker_h *worker, int is_multi_threaded);

// Server-specific function declarations
void recv_callback(void *request, ucs_status_t status, const ucp_tag_recv_info_t *info, void *user_data);
void post_receive(ucp_worker_h worker, client_connection_t *client);
void connection_request_handler(ucp_conn_request_h conn_request, void *arg);

// Server initialization and cleanup
ucs_status_t init_server(server_context_t *server_ctx);
ucs_status_t create_server_listener(server_context_t *server_ctx);
void cleanup_server(server_context_t *server_ctx); 