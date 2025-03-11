#include "rdma_client_helper.h"

// Callback function for request completion
void request_completed(void *request, ucs_status_t status, void *user_data) {
    // Callback function doesn't do anything, letting the main loop handle request completion
}

// Get current time in microseconds
uint64_t get_current_time_us() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

// Send a message using UCX tag interface
int send_message(ucp_worker_h worker, ucp_ep_h ep, const void *buffer, size_t length) {
    ucp_request_param_t send_param;
    memset(&send_param, 0, sizeof(send_param));
    send_param.op_attr_mask = UCP_OP_ATTR_FIELD_CALLBACK | 
                             UCP_OP_ATTR_FIELD_DATATYPE;
    send_param.datatype = ucp_dt_make_contig(1);
    send_param.cb.send = request_completed;

    // Send the message
    ucs_status_ptr_t request = ucp_tag_send_nbx(ep, buffer, length, 0x1337, &send_param);
    
    if (UCS_PTR_IS_ERR(request)) {
        fprintf(stderr, "Send failed: %s\n", ucs_status_string(UCS_PTR_STATUS(request)));
        return -1;
    }

    // Wait for send to complete
    if (UCS_PTR_IS_PTR(request)) {
        while (!ucp_request_is_completed(request)) {
            ucp_worker_progress(worker);
        }
        ucp_request_free(request);
    }

    return length;
}

// Initialize UCP context
ucs_status_t init_ucp_context(ucp_context_h *context) {
    ucp_params_t ucp_params;
    ucp_config_t *config;
    ucs_status_t status;

    // Initialize UCX configuration
    status = ucp_config_read(NULL, NULL, &config);
    if (status != UCS_OK) {
        fprintf(stderr, "ucp_config_read failed\n");
        return status;
    }

    memset(&ucp_params, 0, sizeof(ucp_params));
    ucp_params.field_mask = UCP_PARAM_FIELD_FEATURES;
    ucp_params.features   = UCP_FEATURE_TAG; // Use tag interface

    status = ucp_init(&ucp_params, config, context);
    ucp_config_release(config);
    
    return status;
}

// Initialize UCP worker
ucs_status_t init_ucp_worker(ucp_context_h context, ucp_worker_h *worker, int is_multi_threaded) {
    ucp_worker_params_t worker_params;
    memset(&worker_params, 0, sizeof(worker_params));
    worker_params.field_mask = UCP_WORKER_PARAM_FIELD_THREAD_MODE;
    
    // Set thread mode based on parameter
    if (is_multi_threaded) {
        worker_params.thread_mode = UCS_THREAD_MODE_MULTI;
    } else {
        worker_params.thread_mode = UCS_THREAD_MODE_SINGLE;
    }
    
    return ucp_worker_create(context, &worker_params, worker);
}

// Initialize client
ucs_status_t init_client(client_context_t *client_ctx, const char *server_ip) {
    ucs_status_t status;
    
    // Initialize context
    memset(client_ctx, 0, sizeof(client_context_t));
    
    // Store server IP
    client_ctx->server_ip = strdup(server_ip);
    if (client_ctx->server_ip == NULL) {
        fprintf(stderr, "Failed to allocate memory for server IP\n");
        return UCS_ERR_NO_MEMORY;
    }
    
    // Initialize UCX context
    status = init_ucp_context(&client_ctx->context);
    if (status != UCS_OK) {
        fprintf(stderr, "Failed to initialize UCX context\n");
        free(client_ctx->server_ip);
        return status;
    }
    
    // Create worker (single-threaded mode for client)
    status = init_ucp_worker(client_ctx->context, &client_ctx->worker, 0);
    if (status != UCS_OK) {
        fprintf(stderr, "Failed to create worker\n");
        ucp_cleanup(client_ctx->context);
        free(client_ctx->server_ip);
        return status;
    }
    
    return UCS_OK;
}

// Connect to server
ucs_status_t connect_to_server(client_context_t *client_ctx) {
    ucs_status_t status;
    
    // Construct server socket address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, client_ctx->server_ip, &server_addr.sin_addr);

    // Print connection info for debugging
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &server_addr.sin_addr, ip_str, INET_ADDRSTRLEN);
    printf("Attempting to connect to %s:%d\n", ip_str, ntohs(server_addr.sin_port));

    // Create endpoint using socket address
    ucp_ep_params_t ep_params;
    memset(&ep_params, 0, sizeof(ep_params));
    ep_params.field_mask = UCP_EP_PARAM_FIELD_FLAGS | UCP_EP_PARAM_FIELD_SOCK_ADDR;
    ep_params.flags = UCP_EP_PARAMS_FLAGS_CLIENT_SERVER;
    ep_params.sockaddr.addr = (const struct sockaddr*)&server_addr;
    ep_params.sockaddr.addrlen = sizeof(server_addr);

    status = ucp_ep_create(client_ctx->worker, &ep_params, &client_ctx->ep);
    if (status != UCS_OK) {
        fprintf(stderr, "ucp_ep_create failed\n");
        return status;
    }
    
    printf("Connected to server %s:%d\n", client_ctx->server_ip, SERVER_PORT);

    // Wait for connection to establish
    ucp_ep_params_t ep_params_local;
    memset(&ep_params_local, 0, sizeof(ep_params_local));
    ep_params_local.field_mask = UCP_EP_PARAM_FIELD_ERR_HANDLING_MODE;
    ep_params_local.err_mode = UCP_ERR_HANDLING_MODE_PEER;
    
    // Send a small message to confirm connection is established
    char test_msg[] = "test";
    if (send_message(client_ctx->worker, client_ctx->ep, test_msg, strlen(test_msg)) < 0) {
        fprintf(stderr, "Connection test failed\n");
        return UCS_ERR_CONNECTION_RESET;
    }
    
    // Wait a bit to ensure connection is fully established
    usleep(1000000);  // 1 second
    
    return UCS_OK;
}

// Run performance test
void run_performance_test(client_context_t *client_ctx, int iterations, size_t msg_size) {
    // Prepare test data
    char *send_buf = malloc(msg_size);
    if (send_buf == NULL) {
        fprintf(stderr, "Failed to allocate send buffer\n");
        return;
    }
    
    memset(send_buf, 'A', msg_size);

    // Start performance test
    uint64_t total_time = 0;
    uint64_t start_time, end_time;
    int successful_sends = 0;

    printf("Starting performance test: %d iterations of %zu bytes each\n", 
           iterations, msg_size);

    start_time = get_current_time_us();

    for (int i = 0; i < iterations; i++) {
        if (send_message(client_ctx->worker, client_ctx->ep, send_buf, msg_size) < 0) {
            fprintf(stderr, "Iteration %d failed\n", i);
            continue;
        }

        successful_sends++;
        
        // Print progress every 10 iterations
        if ((i + 1) % 10 == 0) {
            printf("Completed %d/%d iterations\n", i + 1, iterations);
        }
    }

    end_time = get_current_time_us();
    total_time = end_time - start_time;

    // Calculate and print performance statistics
    double total_time_sec = total_time / 1000000.0;
    double throughput = (successful_sends * msg_size) / (1024.0 * 1024.0 * 1024.0) / total_time_sec; // GB/s
    double latency = total_time / (double)successful_sends; // microseconds

    printf("\nPerformance Results:\n");
    printf("Total time: %.3f seconds\n", total_time_sec);
    printf("Successful transfers: %d/%d\n", successful_sends, iterations);
    printf("Average latency: %.2f us\n", latency);
    printf("Throughput: %.2f GB/s\n", throughput);
    printf("Total data transferred: %.2f MB\n", 
           (successful_sends * msg_size) / (1024.0 * 1024.0));
           
    free(send_buf);
}

// Cleanup client resources
void cleanup_client(client_context_t *client_ctx) {
    if (client_ctx->ep != NULL) {
        ucp_ep_destroy(client_ctx->ep);
    }
    
    if (client_ctx->worker != NULL) {
        ucp_worker_destroy(client_ctx->worker);
    }
    
    if (client_ctx->context != NULL) {
        ucp_cleanup(client_ctx->context);
    }
    
    if (client_ctx->server_ip != NULL) {
        free(client_ctx->server_ip);
    }
} 