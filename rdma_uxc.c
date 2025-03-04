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

// Socket functions for address exchange
// The protocol: server sends (addr_len as uint32_t in network order) followed by address bytes.
// Then the client sends its (addr_len and address) back.

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

void usage(const char *prog) {
    printf("Usage: %s -s <bind_ip>   (server mode)\n", prog);
    printf("       %s -c <server_ip> (client mode)\n", prog);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    int is_server = 0;
    const char *ip_arg = NULL;

    if (argc != 3)
        usage(argv[0]);

    if (strcmp(argv[1], "-s") == 0) {
        is_server = 1;
        ip_arg = argv[2];
    } else if (strcmp(argv[1], "-c") == 0) {
        is_server = 0;
        ip_arg = argv[2];
    } else {
        usage(argv[0]);
    }

    /* ----------------- UCX Initialization ----------------- */
    ucp_params_t ucp_params = {0};
    ucp_config_t *config;
    ucs_status_t status;

    // Read UCX config (can be NULL)
    status = ucp_config_read(NULL, NULL, &config);
    if (status != UCS_OK) {
        fprintf(stderr, "Failed to read UCX config\n");
        exit(EXIT_FAILURE);
    }

    ucp_params.field_mask = UCP_PARAM_FIELD_FEATURES;
    // Use tag matching as an example
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
    void *local_addr;
    size_t local_addr_len;
    status = ucp_worker_get_address(worker, &local_addr, &local_addr_len);
    if (status != UCS_OK) {
        fprintf(stderr, "Failed to get worker address\n");
        exit(EXIT_FAILURE);
    }

    /* ----------------- Out-Of-Band Exchange ----------------- */
    int sock;
    if (is_server) {
        int listen_sock = create_server_socket(ip_arg);
        printf("Server listening on %s:%d...\n", ip_arg, SERVER_PORT);
        sock = accept(listen_sock, NULL, NULL);
        if (sock < 0)
            die("accept");
        close(listen_sock);

        // Send our address length and data to client
        uint32_t net_len = htonl(local_addr_len);
        send_all(sock, &net_len, sizeof(net_len));
        send_all(sock, local_addr, local_addr_len);

        // Receive client's worker address length and data
        uint32_t client_addr_len;
        recv_all(sock, &client_addr_len, sizeof(client_addr_len));
        client_addr_len = ntohl(client_addr_len);
        void *remote_addr = malloc(client_addr_len);
        if (!remote_addr)
            die("malloc");
        recv_all(sock, remote_addr, client_addr_len);

        /* Create UCX endpoint using client's address */
        ucp_ep_params_t ep_params = {0};
        ep_params.field_mask = UCP_EP_PARAM_FIELD_REMOTE_ADDRESS;
        ep_params.address    = remote_addr;
        ucp_ep_h ep;
        status = ucp_ep_create(worker, &ep_params, &ep);
        if (status != UCS_OK) {
            fprintf(stderr, "Server: Failed to create endpoint\n");
            exit(EXIT_FAILURE);
        }
        free(remote_addr);
        close(sock);

        /* ----------------- Server: Post a Receive ----------------- */
        char recv_buf[MAX_MESSAGE] = {0};
        /* Note: We use ucp_dt_make_contig(1) for a contiguous datatype. 
         * The function now requires: worker, buffer, count, datatype, tag, tag_mask, callback.
         * We use 0 as the tag_mask to match exactly the given TAG.
         */
        ucs_status_ptr_t recv_req = ucp_tag_recv_nb(worker, recv_buf, MAX_MESSAGE,
                                                     ucp_dt_make_contig(1), TAG, 0, NULL);
        if (UCS_PTR_IS_ERR(recv_req)) {
            fprintf(stderr, "Server: Failed to post receive\n");
            exit(EXIT_FAILURE);
        }
        // Wait for completion
        while (ucp_request_check_status(recv_req) == UCS_INPROGRESS) {
            progress_worker(worker);
        }
        ucp_request_free(recv_req);
        printf("Server received message: %s\n", recv_buf);

        /* Clean up */
        ucp_ep_destroy(ep);
    } else {
        // Client mode: connect to server
        sock = connect_to_server(ip_arg);
        printf("Client connected to server %s:%d\n", ip_arg, SERVER_PORT);

        // Receive server's worker address length and data
        uint32_t server_addr_len;
        recv_all(sock, &server_addr_len, sizeof(server_addr_len));
        server_addr_len = ntohl(server_addr_len);
        void *remote_addr = malloc(server_addr_len);
        if (!remote_addr)
            die("malloc");
        recv_all(sock, remote_addr, server_addr_len);

        // Send our worker address length and data
        uint32_t net_len = htonl(local_addr_len);
        send_all(sock, &net_len, sizeof(net_len));
        send_all(sock, local_addr, local_addr_len);
        close(sock);

        /* Create UCX endpoint using server's address */
        ucp_ep_params_t ep_params = {0};
        ep_params.field_mask = UCP_EP_PARAM_FIELD_REMOTE_ADDRESS;
        ep_params.address    = remote_addr;
        ucp_ep_h ep;
        status = ucp_ep_create(worker, &ep_params, &ep);
        if (status != UCS_OK) {
            fprintf(stderr, "Client: Failed to create endpoint\n");
            exit(EXIT_FAILURE);
        }
        free(remote_addr);

        /* ----------------- Client: Send a Message ----------------- */
        const char *message = "Hello from client";
        ucs_status_ptr_t send_req = ucp_tag_send_nb(ep, message, strlen(message) + 1,
                                                    ucp_dt_make_contig(1), TAG, NULL);
        if (UCS_PTR_IS_ERR(send_req)) {
            fprintf(stderr, "Client: Failed to send message\n");
            exit(EXIT_FAILURE);
        }
        // Wait for completion
        while (ucp_request_check_status(send_req) == UCS_INPROGRESS) {
            progress_worker(worker);
        }
        ucp_request_free(send_req);
        printf("Client sent message: %s\n", message);

        /* Clean up */
        ucp_ep_destroy(ep);
    }

    /* ----------------- Finalize UCX ----------------- */
    ucp_worker_release_address(worker, local_addr);
    ucp_worker_destroy(worker);
    ucp_cleanup(context);
    return 0;
}
