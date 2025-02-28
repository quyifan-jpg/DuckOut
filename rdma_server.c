// rdma_server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <rdma/rdma_cma.h>
#include <infiniband/verbs.h>

#define PORT "12345"
#define BUFFER_SIZE (1024 * 1024)  // 1MB buffer
#define NUM_BUFFERS 4              // Use multiple buffers for pipelining

struct rdma_resources {
    struct rdma_cm_id *cm_id;
    struct ibv_pd *pd;
    struct ibv_cq *cq;
    struct ibv_qp *qp;
    char *recv_regions[NUM_BUFFERS];
    struct ibv_mr *recv_mrs[NUM_BUFFERS];
    int current_buffer;
};

void die(const char *reason) {
    perror(reason);
    exit(EXIT_FAILURE);
}

int main(void) {
    struct rdma_event_channel *ec = rdma_create_event_channel();
    if (!ec)
        die("rdma_create_event_channel");

    struct rdma_cm_id *listener = NULL;
    if (rdma_create_id(ec, &listener, NULL, RDMA_PS_TCP))
        die("rdma_create_id");

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(PORT));
    addr.sin_addr.s_addr = INADDR_ANY;

    if (rdma_bind_addr(listener, (struct sockaddr *)&addr))
        die("rdma_bind_addr");

    if (rdma_listen(listener, 1))
        die("rdma_listen");

    printf("RDMA Server listening on port %s...\n", PORT);

    // Wait for a connection request
    struct rdma_cm_event *event = NULL;
    if (rdma_get_cm_event(ec, &event))
        die("rdma_get_cm_event");
    if (event->event != RDMA_CM_EVENT_CONNECT_REQUEST)
        die("Unexpected event");
    
    struct rdma_cm_id *conn = event->id;
    rdma_ack_cm_event(event);

    // Allocate RDMA resources for the connection
    struct rdma_resources res_rdma = {0};
    res_rdma.cm_id = conn;
    res_rdma.pd = ibv_alloc_pd(conn->verbs);
    if (!res_rdma.pd)
        die("ibv_alloc_pd");
    
    res_rdma.cq = ibv_create_cq(conn->verbs, 10, NULL, NULL, 0);
    if (!res_rdma.cq)
        die("ibv_create_cq");

    struct ibv_qp_init_attr qp_attr = {0};
    qp_attr.send_cq = res_rdma.cq;
    qp_attr.recv_cq = res_rdma.cq;
    qp_attr.qp_type = IBV_QPT_RC;
    qp_attr.cap.max_send_wr = 32;
    qp_attr.cap.max_recv_wr = 32;
    qp_attr.cap.max_send_sge = 1;
    qp_attr.cap.max_recv_sge = 1;
    qp_attr.cap.max_inline_data = 64;

    if (rdma_create_qp(conn, res_rdma.pd, &qp_attr))
        die("rdma_create_qp");
    res_rdma.qp = conn->qp;

    // Allocate multiple receive buffers
    for (int i = 0; i < NUM_BUFFERS; i++) {
        res_rdma.recv_regions[i] = malloc(BUFFER_SIZE);
        if (!res_rdma.recv_regions[i])
            die("malloc recv_region");
        // Clear buffer after allocation
        memset(res_rdma.recv_regions[i], 0, BUFFER_SIZE);
        res_rdma.recv_mrs[i] = ibv_reg_mr(res_rdma.pd, res_rdma.recv_regions[i], 
            BUFFER_SIZE, IBV_ACCESS_LOCAL_WRITE);
        if (!res_rdma.recv_mrs[i])
            die("ibv_reg_mr");
    }

    // Post initial receives for all buffers
    for (int i = 0; i < NUM_BUFFERS; i++) {
        struct ibv_sge sge = {
            .addr = (uintptr_t)res_rdma.recv_regions[i],
            .length = BUFFER_SIZE,
            .lkey = res_rdma.recv_mrs[i]->lkey,
        };
        struct ibv_recv_wr recv_wr = {0}, *bad_recv_wr = NULL;
        recv_wr.sg_list = &sge;
        recv_wr.num_sge = 1;

        if (ibv_post_recv(res_rdma.qp, &recv_wr, &bad_recv_wr))
            die("ibv_post_recv");
    }

    // Accept the connection
    struct rdma_conn_param conn_param = {0};
    if (rdma_accept(conn, &conn_param))
        die("rdma_accept");

    // Wait for connection established event
    if (rdma_get_cm_event(ec, &event))
        die("rdma_get_cm_event");
    if (event->event != RDMA_CM_EVENT_ESTABLISHED)
        die("Connection not established");
    rdma_ack_cm_event(event);
    printf("RDMA connection established.\n");

    // Receive and process data
    struct ibv_wc wc;
    int recv_count = 0;
    
    // First receive is metadata
    while (ibv_poll_cq(res_rdma.cq, 1, &wc) < 1)
        ;
    if (wc.status != IBV_WC_SUCCESS)
        die("Failed receive completion");

    id_t row_count, column_count;
    sscanf(res_rdma.recv_regions[0], "%llu,%llu", &row_count, &column_count);
    printf("\nMetadata received: %llu rows and %llu columns\n\n", row_count, column_count);

    // Print column separator line
    printf("----------------------------------------\n");

    // Receive data rows
    while (recv_count < row_count) {
        while (ibv_poll_cq(res_rdma.cq, 1, &wc) < 1)
            ;
        if (wc.status != IBV_WC_SUCCESS)
            die("Failed receive completion");

        // Process and print received data
        char *data = res_rdma.recv_regions[res_rdma.current_buffer];
        // Remove trailing newline if present
        char *newline = strchr(data, '\n');
        if (newline) *newline = '\0';
        
        printf("Row %d:\n", recv_count);
        
        // Split and print each column value
        char *token = strtok(data, ",");
        int col = 0;
        while (token && col < column_count) {
            printf("  Column %d: %s\n", col, token);
            token = strtok(NULL, ",");
            col++;
        }
        printf("----------------------------------------\n");

        // Clear buffer before posting new receive
        memset(res_rdma.recv_regions[res_rdma.current_buffer], 0, BUFFER_SIZE);

        // Post a new receive for this buffer
        struct ibv_sge sge = {
            .addr = (uintptr_t)res_rdma.recv_regions[res_rdma.current_buffer],
            .length = BUFFER_SIZE,
            .lkey = res_rdma.recv_mrs[res_rdma.current_buffer]->lkey,
        };
        struct ibv_recv_wr recv_wr = {0}, *bad_recv_wr = NULL;
        recv_wr.sg_list = &sge;
        recv_wr.num_sge = 1;

        if (ibv_post_recv(res_rdma.qp, &recv_wr, &bad_recv_wr))
            die("ibv_post_recv");

        // Send acknowledgment back to client
        char ack_msg[64];
        snprintf(ack_msg, sizeof(ack_msg), "ACK_%d", recv_count);
        
        struct ibv_send_wr send_wr = {0}, *bad_send_wr = NULL;
        struct ibv_sge send_sge = {
            .addr = (uintptr_t)res_rdma.recv_regions[res_rdma.current_buffer],
            .length = strlen(ack_msg) + 1,
            .lkey = res_rdma.recv_mrs[res_rdma.current_buffer]->lkey
        };
        
        send_wr.sg_list = &send_sge;
        send_wr.num_sge = 1;
        send_wr.opcode = IBV_WR_SEND;
        send_wr.send_flags = IBV_SEND_SIGNALED;

        // Copy acknowledgment to buffer
        memcpy(res_rdma.recv_regions[res_rdma.current_buffer], ack_msg, strlen(ack_msg) + 1);

        if (ibv_post_send(res_rdma.qp, &send_wr, &bad_send_wr))
            die("ibv_post_send ack");

        // Wait for send completion
        struct ibv_wc wc;
        while (ibv_poll_cq(res_rdma.cq, 1, &wc) < 1)
            ;
        if (wc.status != IBV_WC_SUCCESS)
            die("Failed send completion for ack");

        printf("Sent acknowledgment for row %d\n", recv_count);

        res_rdma.current_buffer = (res_rdma.current_buffer + 1) % NUM_BUFFERS;
        recv_count++;
    }

    // --- Cleanup ---
    rdma_disconnect(conn);
    for (int i = 0; i < NUM_BUFFERS; i++) {
        ibv_dereg_mr(res_rdma.recv_mrs[i]);
        free(res_rdma.recv_regions[i]);
    }
    rdma_destroy_qp(conn);
    ibv_destroy_cq(res_rdma.cq);
    ibv_dealloc_pd(res_rdma.pd);
    rdma_destroy_id(conn);
    rdma_destroy_id(listener);
    rdma_destroy_event_channel(ec);

    return 0;
}
