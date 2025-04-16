#define _POSIX_C_SOURCE 199309L
#include <ucp/api/ucp.h>
#include <ucs/type/status.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <getopt.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/time.h>

// Configuration parameters
#define TEST_MSG_SIZE (64 * 1024 * 1024) // 64MB
#define MIN_MSG_SIZE (1024 * 1024)       // 1MB
#define MAX_MSG_SIZE (64 * 1024 * 1024)  // 64MB
#define ALIGNMENT 64
#define TEST_TAG 0xCAFE
#define SERVER_PORT 13337
#define MAX_QP_NUM 4     // Max QP count
#define QP_TX_DEPTH 2048 // QP send queue depth
#define QP_RX_DEPTH 2048 // QP recv queue depth

// UCX context structure
typedef struct
{
    ucp_context_h context;
    ucp_worker_h worker;
    ucp_ep_h ep;
    ucp_listener_h listener; // For server
} ucx_context_t;

// Callbacks
static void send_handler(void *request, ucs_status_t status)
{
    if (status != UCS_OK)
    {
        fprintf(stderr, "Send completion failed: %s\n", ucs_status_string(status));
    }
}

static void recv_handler(void *request, ucs_status_t status, ucp_tag_recv_info_t *info)
{
    if (status != UCS_OK)
    {
        fprintf(stderr, "Receive completion failed: %s\n", ucs_status_string(status));
    }
}

// Connection handler callback
static void connection_handler(ucp_conn_request_h conn_request, void *arg)
{
    ucx_context_t *ctx = (ucx_context_t *)arg;

    ucp_ep_params_t ep_params;
    memset(&ep_params, 0, sizeof(ep_params));
    ep_params.field_mask = UCP_EP_PARAM_FIELD_CONN_REQUEST;
    ep_params.conn_request = conn_request;

    ucs_status_t status = ucp_ep_create(ctx->worker, &ep_params, &ctx->ep);
    if (status != UCS_OK)
    {
        fprintf(stderr, "Failed to create endpoint from connection request: %s\n",
                ucs_status_string(status));
        return;
    }

    printf("New connection established\n");
}

// Aligned memory allocation
static void *aligned_malloc(size_t size)
{
    void *ptr = NULL;
    if (posix_memalign(&ptr, ALIGNMENT, size) != 0)
    {
        return NULL;
    }
    return ptr;
}

// Print UCX configuration
static void print_ucx_config(ucp_context_h context)
{
    unsigned major_version, minor_version, release_number;
    ucp_get_version(&major_version, &minor_version, &release_number);
    printf("\nUCX Version: %u.%u.%u\n", major_version, minor_version, release_number);

    ucp_config_t *config;
    ucs_status_t status = ucp_config_read(NULL, NULL, &config);
    if (status == UCS_OK)
    {
        printf("\nUCX Configuration:\n");
        ucp_config_print(config, stdout, "UCX", UCS_CONFIG_PRINT_CONFIG);
        ucp_config_release(config);
    }

    printf("\n");
}

/**
 * Initialize UCX context
 *
 * @param ctx Pointer to UCX context
 * @param transport_type Transport type ('tcp' or 'rdma')
 * @return true on success, false on failure
 */
bool ucp_init_context(ucx_context_t *ctx, const char *transport_type)
{
    // Read UCX config
    ucp_config_t *config;
    ucs_status_t status = ucp_config_read(NULL, NULL, &config);
    if (status != UCS_OK)
    {
        fprintf(stderr, "Failed to read UCP config\n");
        return false;
    }

    // 根据传输类型设置不同的配置
    if (strcmp(transport_type, "tcp") == 0)
    {
        // TCP 配置
        status = ucp_config_modify(config, "TLS", "tcp,tcp4,tcp6");
        if (status != UCS_OK)
        {
            fprintf(stderr, "Failed to modify TLS config\n");
            ucp_config_release(config);
            return false;
        }

        status = ucp_config_modify(config, "USE_RDMA_CM", "n");
        if (status != UCS_OK)
        {
            fprintf(stderr, "Failed to modify USE_RDMA_CM config\n");
            ucp_config_release(config);
            return false;
        }

        status = ucp_config_modify(config, "IB_ENABLE", "n");
        if (status != UCS_OK)
        {
            fprintf(stderr, "Failed to modify IB_ENABLE config\n");
            ucp_config_release(config);
            return false;
        }

        status = ucp_config_modify(config, "TCP_CM_ENABLE", "y");
        if (status != UCS_OK)
        {
            fprintf(stderr, "Failed to modify TCP_CM_ENABLE config\n");
            ucp_config_release(config);
            return false;
        }

        status = ucp_config_modify(config, "NET_DEVICES", "lo");
        if (status != UCS_OK)
        {
            fprintf(stderr, "Failed to modify NET_DEVICES config\n");
            ucp_config_release(config);
            return false;
        }

        // TCP 特定参数
        status = ucp_config_modify(config, "TCP_PORT_RANGE", "1000-65000");
        if (status != UCS_OK)
        {
            fprintf(stderr, "Failed to modify TCP_PORT_RANGE config\n");
            ucp_config_release(config);
            return false;
        }

        status = ucp_config_modify(config, "TCP_KEEPIDLE", "75");
        if (status != UCS_OK)
        {
            fprintf(stderr, "Failed to modify TCP_KEEPIDLE config\n");
            ucp_config_release(config);
            return false;
        }

        status = ucp_config_modify(config, "TCP_KEEPCNT", "8");
        if (status != UCS_OK)
        {
            fprintf(stderr, "Failed to modify TCP_KEEPCNT config\n");
            ucp_config_release(config);
            return false;
        }

        status = ucp_config_modify(config, "TCP_KEEPINTVL", "8");
        if (status != UCS_OK)
        {
            fprintf(stderr, "Failed to modify TCP_KEEPINTVL config\n");
            ucp_config_release(config);
            return false;
        }
    }
    else if (strcmp(transport_type, "rdma") == 0)
    {
        // RDMA 配置
        status = ucp_config_modify(config, "TLS", "rc,ud");
        if (status != UCS_OK)
        {
            fprintf(stderr, "Failed to modify TLS config\n");
            ucp_config_release(config);
            return false;
        }

        status = ucp_config_modify(config, "USE_RDMA_CM", "y");
        if (status != UCS_OK)
        {
            fprintf(stderr, "Failed to modify USE_RDMA_CM config\n");
            ucp_config_release(config);
            return false;
        }

        status = ucp_config_modify(config, "IB_ENABLE", "y");
        if (status != UCS_OK)
        {
            fprintf(stderr, "Failed to modify IB_ENABLE config\n");
            ucp_config_release(config);
            return false;
        }

        status = ucp_config_modify(config, "IB_NUM_QPS", "4");
        if (status != UCS_OK)
        {
            fprintf(stderr, "Failed to modify IB_NUM_QPS config\n");
            ucp_config_release(config);
            return false;
        }

        status = ucp_config_modify(config, "IB_TX_QUEUE_LEN", "2048");
        if (status != UCS_OK)
        {
            fprintf(stderr, "Failed to modify IB_TX_QUEUE_LEN config\n");
            ucp_config_release(config);
            return false;
        }

        status = ucp_config_modify(config, "IB_RX_QUEUE_LEN", "2048");
        if (status != UCS_OK)
        {
            fprintf(stderr, "Failed to modify IB_RX_QUEUE_LEN config\n");
            ucp_config_release(config);
            return false;
        }

        status = ucp_config_modify(config, "RC_TX_QUEUE_LEN", "2048");
        if (status != UCS_OK)
        {
            fprintf(stderr, "Failed to modify RC_TX_QUEUE_LEN config\n");
            ucp_config_release(config);
            return false;
        }

        status = ucp_config_modify(config, "RC_RX_QUEUE_LEN", "2048");
        if (status != UCS_OK)
        {
            fprintf(stderr, "Failed to modify RC_RX_QUEUE_LEN config\n");
            ucp_config_release(config);
            return false;
        }
    }
    else
    {
        fprintf(stderr, "Invalid transport type: %s. Use 'tcp' or 'rdma'\n", transport_type);
        ucp_config_release(config);
        return false;
    }

    // Initialize UCP context parameters
    ucp_params_t ucp_params;
    memset(&ucp_params, 0, sizeof(ucp_params));
    ucp_params.field_mask = UCP_PARAM_FIELD_FEATURES |
                            UCP_PARAM_FIELD_REQUEST_SIZE |
                            UCP_PARAM_FIELD_REQUEST_INIT |
                            UCP_PARAM_FIELD_ESTIMATED_NUM_EPS;

    // 根据传输类型设置不同的特性
    if (strcmp(transport_type, "tcp") == 0)
    {
        ucp_params.features = UCP_FEATURE_TAG;
        ucp_params.estimated_num_eps = 1;
    }
    else
    {
        ucp_params.features = UCP_FEATURE_TAG | UCP_FEATURE_RMA | UCP_FEATURE_AM;
        ucp_params.estimated_num_eps = MAX_QP_NUM;
    }

    // Create UCP context
    status = ucp_init(&ucp_params, config, &ctx->context);
    ucp_config_release(config);
    if (status != UCS_OK)
    {
        fprintf(stderr, "Failed to init UCP context: %s\n", ucs_status_string(status));
        return false;
    }

    // Create worker
    ucp_worker_params_t worker_params;
    memset(&worker_params, 0, sizeof(worker_params));
    worker_params.field_mask = UCP_WORKER_PARAM_FIELD_THREAD_MODE;
    worker_params.thread_mode = UCS_THREAD_MODE_SINGLE;

    status = ucp_worker_create(ctx->context, &worker_params, &ctx->worker);
    if (status != UCS_OK)
    {
        fprintf(stderr, "Failed to create worker: %s\n", ucs_status_string(status));
        ucp_cleanup(ctx->context);
        return false;
    }

    ctx->ep = NULL;
    ctx->listener = NULL;
    return true;
}

/**
 * Cleanup UCX resources
 *
 * @param ctx Pointer to UCX context
 */
void ucp_cleanup_context(ucx_context_t *ctx)
{
    if (ctx->ep)
    {
        ucp_ep_destroy(ctx->ep);
    }
    if (ctx->listener)
    {
        ucp_listener_destroy(ctx->listener);
    }
    if (ctx->worker)
    {
        ucp_worker_destroy(ctx->worker);
    }
    if (ctx->context)
    {
        ucp_cleanup(ctx->context);
    }
}

/**
 * Create UCP server
 *
 * @param ctx Pointer to UCX context
 * @param ip IP address to bind to
 * @param port Port to bind to
 * @return true on success, false on failure
 */
bool create_server_ucp(ucx_context_t *ctx, const char *ip, int port)
{
    // Create listen address
    struct sockaddr_in listen_addr;
    memset(&listen_addr, 0, sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &listen_addr.sin_addr);

    // Set listen parameters
    ucp_listener_params_t params;
    memset(&params, 0, sizeof(params));
    params.field_mask = UCP_LISTENER_PARAM_FIELD_SOCK_ADDR |
                        UCP_LISTENER_PARAM_FIELD_CONN_HANDLER;
    params.sockaddr.addr = (const struct sockaddr *)&listen_addr;
    params.sockaddr.addrlen = sizeof(listen_addr);
    params.conn_handler.cb = connection_handler;
    params.conn_handler.arg = ctx;

    // Create listener
    ucs_status_t status = ucp_listener_create(ctx->worker, &params, &ctx->listener);
    if (status != UCS_OK)
    {
        fprintf(stderr, "Failed to create listener: %s\n", ucs_status_string(status));
        return false;
    }

    printf("Server listening on %s:%d\n", ip, port);
    return true;
}

/**
 * Wait for a client connection
 *
 * @param ctx Pointer to UCX context
 * @param timeout_ms Timeout in milliseconds, 0 for no timeout
 * @return true when connected, false on timeout or failure
 */
bool ucp_wait_for_connection(ucx_context_t *ctx, int timeout_ms)
{
    struct timespec start, current;
    clock_gettime(CLOCK_MONOTONIC, &start);

    while (ctx->ep == NULL)
    {
        ucp_worker_progress(ctx->worker);

        // Check for timeout
        if (timeout_ms > 0)
        {
            clock_gettime(CLOCK_MONOTONIC, &current);
            double elapsed = (current.tv_sec - start.tv_sec) * 1000 +
                             (current.tv_nsec - start.tv_nsec) / 1000000.0;
            if (elapsed > timeout_ms)
            {
                fprintf(stderr, "Timeout waiting for connection\n");
                return false;
            }
        }

        usleep(1000); // 1ms
    }

    return true;
}

/**
 * Create UCP client and connect to server
 *
 * @param ctx Pointer to UCX context
 * @param server_ip Server IP address
 * @param port Server port
 * @return true on success, false on failure
 */
bool create_client_ucp(ucx_context_t *ctx, const char *server_ip, int port)
{
    struct sockaddr_in connect_addr;
    memset(&connect_addr, 0, sizeof(connect_addr));
    connect_addr.sin_family = AF_INET;
    connect_addr.sin_port = htons(port);
    inet_pton(AF_INET, server_ip, &connect_addr.sin_addr);

    ucp_ep_params_t ep_params;
    memset(&ep_params, 0, sizeof(ep_params));
    ep_params.field_mask = UCP_EP_PARAM_FIELD_FLAGS |
                           UCP_EP_PARAM_FIELD_SOCK_ADDR;
    ep_params.flags = UCP_EP_PARAMS_FLAGS_CLIENT_SERVER;
    ep_params.sockaddr.addr = (const struct sockaddr *)&connect_addr;
    ep_params.sockaddr.addrlen = sizeof(connect_addr);

    ucs_status_t status = ucp_ep_create(ctx->worker, &ep_params, &ctx->ep);
    if (status != UCS_OK)
    {
        fprintf(stderr, "Failed to create endpoint: %s\n", ucs_status_string(status));
        return false;
    }

    // Wait for connection to establish
    ucp_worker_progress(ctx->worker);
    sleep(1); // Give connection some time to establish

    printf("Connected to server %s:%d\n", server_ip, port);
    return true;
}

/**
 * Send data using UCP
 *
 * @param ctx Pointer to UCX context
 * @param buffer Data buffer to send
 * @param length Length of data to send
 * @param tag Tag for the message
 * @return true on success, false on failure
 */
bool ucp_send_data(ucx_context_t *ctx, const void *buffer, size_t length, ucp_tag_t tag)
{
    void *req = ucp_tag_send_nb(ctx->ep, buffer, length,
                                ucp_dt_make_contig(1), tag,
                                send_handler);
    if (UCS_PTR_IS_ERR(req))
    {
        fprintf(stderr, "Error sending data: %s\n", ucs_status_string(UCS_PTR_STATUS(req)));
        return false;
    }

    // Wait for completion
    if (UCS_PTR_IS_PTR(req))
    {
        while (1)
        {
            ucp_worker_progress(ctx->worker);
            ucs_status_t status = ucp_request_check_status(req);
            if (status != UCS_INPROGRESS)
            {
                ucp_request_free(req);
                if (status != UCS_OK)
                {
                    fprintf(stderr, "Failed to send data: %s\n", ucs_status_string(status));
                    return false;
                }
                break;
            }
        }
    }

    return true;
}

/**
 * Receive data using UCP
 *
 * @param ctx Pointer to UCX context
 * @param buffer Buffer to receive data into
 * @param length Maximum length of data to receive
 * @param tag Tag to match
 * @param tag_mask Mask for tag matching
 * @param received_length Pointer to store actual received data length
 * @return true on success, false on failure
 */
bool ucp_recv_data(ucx_context_t *ctx, void *buffer, size_t length,
                   ucp_tag_t tag, ucp_tag_t tag_mask, size_t *received_length)
{
    ucp_tag_recv_info_t info_tag;
    void *req = ucp_tag_recv_nb(ctx->worker, buffer, length,
                                ucp_dt_make_contig(1), tag, tag_mask,
                                recv_handler);

    if (UCS_PTR_IS_ERR(req))
    {
        fprintf(stderr, "Error receiving data: %s\n", ucs_status_string(UCS_PTR_STATUS(req)));
        return false;
    }

    // Wait for completion
    if (UCS_PTR_IS_PTR(req))
    {
        while (1)
        {
            ucp_worker_progress(ctx->worker);
            ucs_status_t status = ucp_request_check_status(req);
            if (status != UCS_INPROGRESS)
            {
                ucp_request_free(req);
                if (status != UCS_OK)
                {
                    fprintf(stderr, "Failed to receive data: %s\n", ucs_status_string(status));
                    return false;
                }
                break;
            }
        }
    }
    printf("received_length is %zu\n", *received_length);
    if (received_length)
    {
        *received_length = length; // In a real implementation, get the actual length from info_tag
    }

    return true;
}

/**
 * Run performance test as server with one-way communication
 * Server just receives data from client, no responses sent back
 */
static bool run_server_test(const char *ip, int port)
{
    ucx_context_t ctx;
    if (!ucp_init_context(&ctx, "tcp"))
    {
        return false;
    }

    if (!create_server_ucp(&ctx, ip, port))
    {
        ucp_cleanup_context(&ctx);
        return false;
    }

    // Wait for connection
    if (!ucp_wait_for_connection(&ctx, 30000)) // 30 second timeout
    {
        ucp_cleanup_context(&ctx);
        return false;
    }

    // Allocate buffer
    char *buffer = static_cast<char *>(aligned_malloc(TEST_MSG_SIZE));
    if (buffer == NULL)
    {
        fprintf(stderr, "Failed to allocate aligned memory\n");
        ucp_cleanup_context(&ctx);
        return false;
    }

    struct timespec start, end;
    double total_time = 0;
    size_t total_bytes = 0;
    int msg_count = 0;

    printf("Server ready to receive data (one-way communication)\n");

    // Process messages - only receiving, no responses
    while (1)
    {
        clock_gettime(CLOCK_MONOTONIC, &start);

        // Receive data
        size_t recv_len;
        if (!ucp_recv_data(&ctx, buffer, TEST_MSG_SIZE, TEST_TAG, (ucp_tag_t)-1, &recv_len))
        {
            break;
        }

        clock_gettime(CLOCK_MONOTONIC, &end);
        double elapsed = (end.tv_sec - start.tv_sec) +
                         (end.tv_nsec - start.tv_nsec) / 1e9;

        total_time += elapsed;
        total_bytes += TEST_MSG_SIZE;
        msg_count++;

        // Show stats every message
        double current_bandwidth = (TEST_MSG_SIZE / elapsed) / (1024 * 1024);
        printf("Message %d: Size: %.2f MB, Time: %.6f s, Bandwidth: %.2f MB/s\n",
               msg_count, TEST_MSG_SIZE / (1024.0 * 1024.0), elapsed, current_bandwidth);

        // Check if we received a termination message (first byte is 0)
        if (buffer[0] == 0)
        {
            printf("Received termination message, exiting\n");
            break;
        }
    }

    // Print overall statistics
    if (msg_count > 0)
    {
        double avg_bandwidth = (total_bytes / total_time) / (1024 * 1024);
        printf("\nSummary Statistics:\n");
        printf("Total messages: %d\n", msg_count);
        printf("Total data received: %.2f MB\n", total_bytes / (1024.0 * 1024.0));
        printf("Average bandwidth: %.2f MB/s\n", avg_bandwidth);
    }

    free(buffer);
    ucp_cleanup_context(&ctx);
    return true;
}

/**
 * Run performance test as client with one-way communication
 * Client only sends data, no responses expected
 */
static bool run_client_test(const char *server_ip, int port)
{
    ucx_context_t ctx;
    if (!ucp_init_context(&ctx, "tcp"))
    {
        return false;
    }

    if (!create_client_ucp(&ctx, server_ip, port))
    {
        ucp_cleanup_context(&ctx);
        return false;
    }

    // Allocate send buffer
    char *send_buffer = static_cast<char *>(aligned_malloc(TEST_MSG_SIZE));
    if (send_buffer == NULL)
    {
        fprintf(stderr, "Failed to allocate aligned memory\n");
        ucp_cleanup_context(&ctx);
        return false;
    }

    // Fill test data
    memset(send_buffer, 'A', TEST_MSG_SIZE);

    const int iterations = 1000;
    struct timespec start, end;
    double total_time = 0;

    printf("Client starting one-way data transfer (%d messages)\n", iterations);

    clock_gettime(CLOCK_MONOTONIC, &start);

    // Send messages without waiting for responses
    for (int i = 0; i < iterations; ++i)
    {
        struct timespec msg_start, msg_end;
        clock_gettime(CLOCK_MONOTONIC, &msg_start);

        // Mark the first byte with the iteration number (for identification)
        // This helps server track individual messages
        send_buffer[0] = (char)(i % 255) + 1; // Avoid 0 which is termination signal

        // Send data
        if (!ucp_send_data(&ctx, send_buffer, TEST_MSG_SIZE, TEST_TAG))
        {
            fprintf(stderr, "Failed to send message %d\n", i);
            break;
        }

        clock_gettime(CLOCK_MONOTONIC, &msg_end);
        double msg_time = (msg_end.tv_sec - msg_start.tv_sec) +
                          (msg_end.tv_nsec - msg_start.tv_nsec) / 1e9;
        total_time += msg_time;

        if (i % 100 == 0)
        {
            double current_bandwidth = (TEST_MSG_SIZE / msg_time) / (1024 * 1024);
            printf("Sent message %d/%d: Bandwidth: %.2f MB/s\n",
                   i, iterations, current_bandwidth);
        }
    }

    // Send termination message (first byte set to 0)
    send_buffer[0] = 0;
    ucp_send_data(&ctx, send_buffer, TEST_MSG_SIZE, TEST_TAG);
    printf("Sent termination message\n");

    clock_gettime(CLOCK_MONOTONIC, &end);
    double duration = (end.tv_sec - start.tv_sec) +
                      (end.tv_nsec - start.tv_nsec) / 1e9;

    double avg_msg_time = total_time / iterations;
    double bandwidth = (TEST_MSG_SIZE * iterations) /
                       (duration * 1024 * 1024);

    printf("\nResults:\n");
    printf("Message size: %.2f MB\n", TEST_MSG_SIZE / (1024.0 * 1024.0));
    printf("Total messages: %d\n", iterations);
    printf("Average message send time: %.6f seconds\n", avg_msg_time);
    printf("Overall bandwidth: %.2f MB/s\n", bandwidth);

    free(send_buffer);
    ucp_cleanup_context(&ctx);
    return true;
}
