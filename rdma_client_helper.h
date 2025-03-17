#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ucp/api/ucp.h>
#include <ucs/config/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>

// Client-specific definitions
#define SERVER_PORT 13337
#define TEST_ITERATIONS 100  // Number of test iterations

// Client context structure
typedef struct {
    ucp_context_h context;
    ucp_worker_h worker;
    ucp_ep_h ep;
    char *server_ip;
} client_context_t;

// Common utility functions
void request_completed(void *request, ucs_status_t status, void *user_data);
uint64_t get_current_time_us();
int send_message(ucp_worker_h worker, ucp_ep_h ep, const void *buffer, size_t length);

// UCX initialization and cleanup functions
ucs_status_t init_ucp_context(ucp_context_h *context);
ucs_status_t init_ucp_worker(ucp_context_h context, ucp_worker_h *worker, int is_multi_threaded);

// Client-specific function declarations
ucs_status_t init_client(client_context_t *client_ctx, const char *server_ip);
ucs_status_t connect_to_server(client_context_t *client_ctx);
void run_performance_test(client_context_t *client_ctx, int iterations, size_t msg_size);
void cleanup_client(client_context_t *client_ctx); 