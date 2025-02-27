// rdma_server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <rdma/rdma_cma.h>
#include <infiniband/verbs.h>

#define PORT "12345"
#define BUFFER_SIZE 4096

struct rdma_resources {
    struct rdma_cm_id *cm_id;
    struct ibv_pd *pd;
    struct ibv_cq *cq;
    struct ibv_qp *qp;
    char *recv_region;
    struct ibv_mr *recv_mr;
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
    qp_attr.cap.max_send_wr = 10;
    qp_attr.cap.max_recv_wr = 10;
    qp_attr.cap.max_send_sge = 1;
    qp_attr.cap.max_recv_sge = 1;

    if (rdma_create_qp(conn, res_rdma.pd, &qp_attr))
        die("rdma_create_qp");
    res_rdma.qp = conn->qp;

    // Allocate and register a receive buffer
    res_rdma.recv_region = malloc(BUFFER_SIZE);
    if (!res_rdma.recv_region)
        die("malloc recv_region");
    res_rdma.recv_mr = ibv_reg_mr(res_rdma.pd, res_rdma.recv_region, BUFFER_SIZE,
                                  IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE);
    if (!res_rdma.recv_mr)
        die("ibv_reg_mr");

    // Post a receive work request
    struct ibv_sge sge = {
        .addr = (uintptr_t)res_rdma.recv_region,
        .length = BUFFER_SIZE,
        .lkey = res_rdma.recv_mr->lkey,
    };
    struct ibv_recv_wr recv_wr = {0}, *bad_recv_wr = NULL;
    recv_wr.sg_list = &sge;
    recv_wr.num_sge = 1;

    if (ibv_post_recv(res_rdma.qp, &recv_wr, &bad_recv_wr))
        die("ibv_post_recv");

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

    // --- RDMA Data Reception ---
    // Now poll the completion queue for a receive completion
    struct ibv_wc wc;
    while (ibv_poll_cq(res_rdma.cq, 1, &wc) < 1)
        ;
    if (wc.status != IBV_WC_SUCCESS)
        die("Failed receive completion");

    printf("Received message: %s\n", res_rdma.recv_region);

    // You would repeat posting receives and handling messages for the metadata and rows.

    // --- Cleanup ---
    rdma_disconnect(conn);
    ibv_dereg_mr(res_rdma.recv_mr);
    free(res_rdma.recv_region);
    rdma_destroy_qp(conn);
    ibv_destroy_cq(res_rdma.cq);
    ibv_dealloc_pd(res_rdma.pd);
    rdma_destroy_id(conn);
    rdma_destroy_id(listener);
    rdma_destroy_event_channel(ec);

    return 0;
}
