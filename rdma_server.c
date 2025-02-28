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
    char *ack_buffer;           // Add separate ack buffer
    struct ibv_mr *ack_mr;      // Add separate MR for ack
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

    // Allocate separate buffer for acknowledgments
    res_rdma.ack_buffer = malloc(64);  // Small buffer for acks
    if (!res_rdma.ack_buffer)
        die("malloc ack_buffer");
    res_rdma.ack_mr = ibv_reg_mr(res_rdma.pd, res_rdma.ack_buffer, 
        64, IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE);
    if (!res_rdma.ack_mr)
        die("ibv_reg_mr ack");

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
    sscanf(res_rdma.recv_regions[0], "%u,%u", &row_count, &column_count);
    printf("\nMetadata received: %u rows and %u columns\n\n", row_count, column_count);

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
        // while (token && col < column_count) {
        //     printf("  Column %d: %s\n", col, token);
        //     token = strtok(NULL, ",");
        //     col++;
        // }
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
        snprintf(res_rdma.ack_buffer, 64, "ACK_%d", recv_count);
        
        // Add more detailed debug prints
        printf("----------------------------------------\n");
        printf("Ack buffer contents: '%s'\n", res_rdma.ack_buffer);
        printf("Ack buffer length: %zu\n", strlen(res_rdma.ack_buffer) + 1);
        
        struct ibv_send_wr send_wr = {0}, *bad_send_wr = NULL;
        struct ibv_sge send_sge = {
            .addr = (uintptr_t)res_rdma.ack_buffer,
            .length = strlen(res_rdma.ack_buffer) + 1,
            .lkey = res_rdma.ack_mr->lkey
        };

        send_wr.wr_id = recv_count;
        send_wr.opcode = IBV_WR_SEND;
        send_wr.sg_list = &send_sge;
        send_wr.num_sge = 1;
        send_wr.send_flags = IBV_SEND_SIGNALED;

        // Add debug prints for memory registration
        printf("Memory registration debug:\n");
        printf("  ack_buffer address: %p\n", res_rdma.ack_buffer);
        printf("  ack_mr address: %p\n", res_rdma.ack_mr);
        printf("  ack_mr->lkey: 0x%x\n", res_rdma.ack_mr->lkey);
        printf("  send_sge.addr: 0x%lx\n", send_sge.addr);
        printf("  send_sge.length: %d\n", send_sge.length);
        printf("  send_sge.lkey: 0x%x\n", send_sge.lkey);

        // Verify memory region exists
        if (res_rdma.ack_mr == NULL) {
            printf("  ERROR: ack_mr is NULL!\n");
        }

        printf("Posting send for ACK_%d\n", recv_count);
        int ret = ibv_post_send(res_rdma.qp, &send_wr, &bad_send_wr);
        if (ret) {
            printf("Error posting send: %s (%d)\n", strerror(errno), errno);
            break;
        }

        // Wait for send completion with timeout
        struct ibv_wc wc;
        int ne;
        int poll_count = 0;
        const int MAX_POLL_CQ_ATTEMPTS = 1000000;

        printf("Waiting for send completion for ACK_%d...\n", recv_count);
        do {
            ne = ibv_poll_cq(res_rdma.cq, 1, &wc);
            if (ne < 0) {
                printf("Error polling CQ: %s\n", strerror(errno));
                break;
            }
            if (++poll_count >= MAX_POLL_CQ_ATTEMPTS) {
                printf("Timeout waiting for send completion\n");
                break;
            }
            
            // Add small delay between polls to prevent CPU spinning
            
        } while (ne < 1);

        if (ne > 0) {
            if (wc.status == IBV_WC_SUCCESS) {
                if (wc.wr_id != recv_count) {
                    printf("WARNING: Completed wr_id %ld when expecting %d\n", 
                           wc.wr_id, recv_count);
                }
                printf("Send completed successfully for ACK_%ld\n", wc.wr_id);
            } else {
                printf("Send completion failed with status: %s (%d)\n", 
                       ibv_wc_status_str(wc.status), wc.status);
                break;
            }
        }
        printf("----------------------------------------\n");

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
    ibv_dereg_mr(res_rdma.ack_mr);
    free(res_rdma.ack_buffer);

    return 0;
}
