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

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        die("connect");

    return sock;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <server_ip>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *server_ip = argv[1];

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
    int sock = connect_to_server(server_ip);
    printf("Connected to server %s:%d\n", server_ip, SERVER_PORT);

    // Receive server's worker address
    uint32_t server_addr_len;
    recv_all(sock, &server_addr_len, sizeof(server_addr_len));
    server_addr_len = ntohl(server_addr_len);
    void *remote_addr = malloc(server_addr_len);
    if (!remote_addr)
        die("malloc");
    recv_all(sock, remote_addr, server_addr_len);

    // Send our worker address
    uint32_t net_len = htonl(local_addr_len);
    send_all(sock, &net_len, sizeof(net_len));
    send_all(sock, local_addr, local_addr_len);
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

    /* ----------------- Send Message ----------------- */
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
        exit(EXIT_FAILURE);
    }

    if (send_req != NULL) {
        while (ucp_request_check_status(send_req) == UCS_INPROGRESS) {
            progress_worker(worker);
        }
        ucp_request_free(send_req);
    }

    printf("Sent RDMA message of size %d bytes\n", MAX_MESSAGE);

    /* Clean up */
    ucp_ep_destroy(ep);
    ucp_worker_release_address(worker, local_addr);
    ucp_worker_destroy(worker);
    ucp_cleanup(context);

    return 0;
}