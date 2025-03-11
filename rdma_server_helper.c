#include "rdma_server_helper.h"

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

// Receive callback function
void recv_callback(void *request, ucs_status_t status, const ucp_tag_recv_info_t *info, void *user_data) {
    size_t *size = (size_t *)user_data;
    *size = info->length;
}

// Post receive function
void post_receive(ucp_worker_h worker, client_connection_t *client) {
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
        fprintf(stderr, "Receive request failed: %s\n", 
                ucs_status_string(UCS_PTR_STATUS(request)));
        client->active = 0;
        return;
    }

    if (UCS_PTR_IS_PTR(request)) {
        ucp_request_free(request);
    }
}

// Connection request handler callback
void connection_request_handler(ucp_conn_request_h conn_request, void *arg) {
    server_context_t *server_ctx = (server_context_t *)arg;
    
    if (server_ctx->num_clients >= MAX_CLIENTS) {
        fprintf(stderr, "Maximum number of clients reached\n");
        return;
    }

    // Find an available client slot
    int client_idx = server_ctx->num_clients++;
    client_connection_t *client = &server_ctx->clients[client_idx];

    // Create endpoint
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

    // Post receive request for the new client
    post_receive(server_ctx->worker, client);
}

// Initialize server
ucs_status_t init_server(server_context_t *server_ctx) {
    ucs_status_t status;
    
    // Initialize context
    memset(server_ctx, 0, sizeof(server_context_t));
    status = init_ucp_context(&server_ctx->context);
    if (status != UCS_OK) {
        fprintf(stderr, "Failed to initialize UCX context\n");
        return status;
    }
    
    // Create worker (multi-threaded mode for server)
    status = init_ucp_worker(server_ctx->context, &server_ctx->worker, 1);
    if (status != UCS_OK) {
        fprintf(stderr, "Failed to create worker\n");
        ucp_cleanup(server_ctx->context);
        return status;
    }
    
    return UCS_OK;
}

// Create listener for server
ucs_status_t create_server_listener(server_context_t *server_ctx) {
    ucs_status_t status;
    
    // Set up listening socket address
    struct sockaddr_in listen_addr;
    memset(&listen_addr, 0, sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = INADDR_ANY;
    listen_addr.sin_port = htons(SERVER_PORT);

    ucp_listener_params_t listener_params;
    memset(&listener_params, 0, sizeof(listener_params));
    listener_params.field_mask = UCP_LISTENER_PARAM_FIELD_SOCK_ADDR | UCP_LISTENER_PARAM_FIELD_CONN_HANDLER;
    listener_params.sockaddr.addr = (struct sockaddr *)&listen_addr;
    listener_params.sockaddr.addrlen = sizeof(listen_addr);
    listener_params.conn_handler.cb = connection_request_handler;
    listener_params.conn_handler.arg = server_ctx;

    status = ucp_listener_create(server_ctx->worker, &listener_params, &server_ctx->listener);
    if (status != UCS_OK) {
        fprintf(stderr, "ucp_listener_create failed: %s\n", ucs_status_string(status));
        return status;
    }
    
    printf("Server listening on port %d...\n", SERVER_PORT);
    return UCS_OK;
}

// Cleanup server resources
void cleanup_server(server_context_t *server_ctx) {
    if (server_ctx->listener != NULL) {
        ucp_listener_destroy(server_ctx->listener);
    }
    
    // Clean up all client endpoints
    for (int i = 0; i < server_ctx->num_clients; i++) {
        if (server_ctx->clients[i].active) {
            ucp_ep_destroy(server_ctx->clients[i].ep);
        }
    }
    
    if (server_ctx->worker != NULL) {
        ucp_worker_destroy(server_ctx->worker);
    }
    
    if (server_ctx->context != NULL) {
        ucp_cleanup(server_ctx->context);
    }
} 