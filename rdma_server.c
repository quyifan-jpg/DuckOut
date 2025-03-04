#include "rdma_common.h"

static int create_server_socket(const char *bind_ip) {
    int sock;
    struct sockaddr_in addr;
    int opt = 1;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        die("socket");

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        die("setsockopt");

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(SERVER_PORT);
    if (inet_pton(AF_INET, bind_ip, &addr.sin_addr) <= 0)
        die("inet_pton");

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        die("bind");

    if (listen(sock, 1) < 0)
        die("listen");

    return sock;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <bind_ip>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *bind_ip = argv[1];

    /* ----------------- UCX Initialization ----------------- */
    ucp_params_t ucp_params = {0};
    ucp_config_t *config;
    ucs_status_t status;

    status = ucp_config_read(NULL, NULL, &config);
    if (status != UCS_OK) {
        fprintf(stderr, "Failed to read UCX config\n");
        exit(EXIT_FAILURE);
    }

    ucp_params.field_mask = UCP_PARAM_FIELD_FEATURES;
    ucp_params.features   = UCP_FEATURE_TAG;

    ucp_context_h context;
    status = ucp_init(&ucp_params, config, &context);
    if (status != UCS_OK) {
        fprintf(stderr, "Failed to initialize UCX\n");
        exit(EXIT_FAILURE);
    }
    ucp_config_release(config);

    ucp_worker_params_t worker_params = {0};
    worker_params.field_mask = UCP_WORKER_PARAM_FIELD_THREAD_MODE;
    worker_params.thread_mode = UCS_THREAD_MODE_SINGLE;

    ucp_worker_h worker;
    status = ucp_worker_create(context, &worker_params, &worker);
    if (status != UCS_OK) {
        fprintf(stderr, "Failed to create UCX worker\n");
        exit(EXIT_FAILURE);
    }

    /* Get local worker address */
    ucp_address_t *local_addr;
    size_t local_addr_len;
    status = ucp_worker_get_address(worker, &local_addr, &local_addr_len);
    if (status != UCS_OK) {
        fprintf(stderr, "Failed to get worker address\n");
        exit(EXIT_FAILURE);
    }

    /* ----------------- Address Exchange ----------------- */
    int listen_sock = create_server_socket(bind_ip);
    printf("Server listening on %s:%d...\n", bind_ip, SERVER_PORT);
    
    int sock = accept(listen_sock, NULL, NULL);
    if (sock < 0)
        die("accept");
    close(listen_sock);

    // Send our address length and data to client
    uint32_t net_len = htonl(local_addr_len);
    send_all(sock, &net_len, sizeof(net_len));
    send_all(sock, local_addr, local_addr_len);

    // Receive client's worker address
    uint32_t client_addr_len;
    recv_all(sock, &client_addr_len, sizeof(client_addr_len));
    client_addr_len = ntohl(client_addr_len);
    void *remote_addr = malloc(client_addr_len);
    if (!remote_addr)
        die("malloc");
    recv_all(sock, remote_addr, client_addr_len);
    close(sock);

    /* Create endpoint */
    ucp_ep_params_t ep_params = {0};
    ep_params.field_mask = UCP_EP_PARAM_FIELD_REMOTE_ADDRESS;
    ep_params.address    = remote_addr;
    ucp_ep_h ep;
    status = ucp_ep_create(worker, &ep_params, &ep);
    if (status != UCS_OK) {
        fprintf(stderr, "Failed to create endpoint\n");
        exit(EXIT_FAILURE);
    }
    free(remote_addr);

    /* ----------------- Receive Message ----------------- */
    char recv_buf[MAX_MESSAGE] = {0};
    
    ucp_request_param_t param = {0};
    param.op_attr_mask = UCP_OP_ATTR_FIELD_CALLBACK;
    param.cb.recv      = NULL;

    ucs_status_ptr_t recv_req = ucp_tag_recv_nbx(worker, recv_buf, MAX_MESSAGE,
                                                TAG, 0, &param);
    if (UCS_PTR_IS_ERR(recv_req)) {
        fprintf(stderr, "Failed to post receive\n");
        exit(EXIT_FAILURE);
    }

    if (recv_req != NULL) {
        while (ucp_request_check_status(recv_req) == UCS_INPROGRESS) {
            progress_worker(worker);
        }
        ucp_request_free(recv_req);
    }

    printf("Server received message: %s\n", recv_buf);

    /* Clean up */
    ucp_ep_destroy(ep);
    ucp_worker_release_address(worker, local_addr);
    ucp_worker_destroy(worker);
    ucp_cleanup(context);

    return 0;
}