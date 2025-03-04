#include "rdma_common.h"

static int connect_to_server(const char *server_ip) {
    int sock;
    struct sockaddr_in addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        die("socket");

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(SERVER_PORT);
    if (inet_pton(AF_INET, server_ip, &addr.sin_addr) <= 0)
        die("inet_pton");

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "Connect failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    return sock;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <server_ip>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *server_ip = argv[1];
    ucp_context_h context = NULL;
    ucp_worker_h worker = NULL;
    ucp_ep_h ep = NULL;
    void *local_addr = NULL;
    void *remote_addr = NULL;
    int sock = -1;

    /* ----------------- UCX Initialization ----------------- */
    ucp_params_t ucp_params = {0};
    ucp_config_t *config;
    ucs_status_t status;

    status = ucp_config_read(NULL, NULL, &config);
    if (status != UCS_OK) {
        fprintf(stderr, "Failed to read UCX config\n");
        goto cleanup;
    }

    ucp_params.field_mask = UCP_PARAM_FIELD_FEATURES;
    ucp_params.features   = UCP_FEATURE_TAG;

    status = ucp_init(&ucp_params, config, &context);
    ucp_config_release(config);
    if (status != UCS_OK) {
        fprintf(stderr, "Failed to init UCX context\n");
        goto cleanup;
    }

    ucp_worker_params_t worker_params = {0};
    worker_params.field_mask = UCP_WORKER_PARAM_FIELD_THREAD_MODE;
    worker_params.thread_mode = UCS_THREAD_MODE_SINGLE;

    status = ucp_worker_create(context, &worker_params, &worker);
    if (status != UCS_OK) {
        fprintf(stderr, "Failed to create worker\n");
        goto cleanup;
    }

    /* Get worker address */
    size_t local_addr_len;
    status = ucp_worker_get_address(worker, (ucp_address_t **)&local_addr, &local_addr_len);
    if (status != UCS_OK) {
        fprintf(stderr, "Failed to get worker address\n");
        goto cleanup;
    }

    /* Exchange addresses */
    sock = connect_to_server(server_ip);
    
    // Send our address length and address
    uint32_t net_len = htonl(local_addr_len);
    send_all(sock, &net_len, sizeof(net_len));
    send_all(sock, local_addr, local_addr_len);

    // Receive server's address
    uint32_t server_addr_len;
    recv_all(sock, &server_addr_len, sizeof(server_addr_len));
    server_addr_len = ntohl(server_addr_len);
    
    remote_addr = malloc(server_addr_len);
    if (!remote_addr) {
        fprintf(stderr, "Failed to allocate memory for remote address\n");
        goto cleanup;
    }
    recv_all(sock, remote_addr, server_addr_len);
    close(sock);
    sock = -1;

    /* Create endpoint */
    ucp_ep_params_t ep_params = {0};
    ep_params.field_mask = UCP_EP_PARAM_FIELD_REMOTE_ADDRESS;
    ep_params.address    = remote_addr;

    status = ucp_ep_create(worker, &ep_params, &ep);
    if (status != UCS_OK) {
        fprintf(stderr, "Failed to create endpoint\n");
        goto cleanup;
    }

    /* Send the message */
    char send_buffer[MAX_MESSAGE];
    memset(send_buffer, 'A', MAX_MESSAGE - 1);
    send_buffer[MAX_MESSAGE - 1] = '\0';

    ucp_request_param_t param = {0};
    param.op_attr_mask = UCP_OP_ATTR_FIELD_CALLBACK;
    param.cb.send      = NULL;

    ucs_status_ptr_t send_req = ucp_tag_send_nbx(ep, send_buffer, MAX_MESSAGE,
                                                TAG, &param);
    if (UCS_PTR_IS_ERR(send_req)) {
        fprintf(stderr, "Failed to send message\n");
        goto cleanup;
    }

    if (send_req != NULL) {
        while (ucp_request_check_status(send_req) == UCS_INPROGRESS) {
            ucp_worker_progress(worker);
        }
        ucp_request_free(send_req);
    }

    printf("Message sent successfully\n");

    /* Ensure the message is delivered before cleanup */
    ucp_worker_flush(worker);

cleanup:
    if (ep != NULL) {
        ucp_ep_destroy(ep);
    }
    if (local_addr != NULL) {
        ucp_worker_release_address(worker, local_addr);
    }
    if (remote_addr != NULL) {
        free(remote_addr);
    }
    if (worker != NULL) {
        ucp_worker_destroy(worker);
    }
    if (context != NULL) {
        ucp_cleanup(context);
    }
    if (sock >= 0) {
        close(sock);
    }

    return 0;
}