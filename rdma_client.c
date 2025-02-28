// rdma_client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <rdma/rdma_cma.h>
#include <infiniband/verbs.h>
#include <duckdb.h>

#define PORT "12345"  // Use a string port for rdma_resolve_addr
#define BUFFER_SIZE 4096

// Structure to hold RDMA resources
struct rdma_resources {
    struct rdma_cm_id *cm_id;
    struct ibv_pd *pd;
    struct ibv_cq *cq;
    struct ibv_qp *qp;
    char *send_region;
    struct ibv_mr *send_mr;
};

void die(const char *reason) {
    perror(reason);
    exit(EXIT_FAILURE);
}

int main(void) {
    // --- DuckDB setup (same as before) ---
    duckdb_database db;
    duckdb_connection con;
    duckdb_state status;
    duckdb_result res;
    const char *db_path = "tpch.duckdb";
    
    status = duckdb_open(db_path, &db);
    if (status != DuckDBSuccess) {
        fprintf(stderr, "Failed to open DuckDB database\n");
        exit(EXIT_FAILURE);
    }
    status = duckdb_connect(db, &con);
    status = duckdb_query(con, "SELECT * FROM lineitem", &res);
    if (status != DuckDBSuccess) {
        fprintf(stderr, "Failed to execute query\n");
        exit(EXIT_FAILURE);
    }
    idx_t row_count = duckdb_row_count(&res);
    idx_t column_count = duckdb_column_count(&res);
    printf("Query returned %llu rows and %llu columns\n", row_count, column_count);

    // --- RDMA Setup ---
    struct rdma_event_channel *ec = rdma_create_event_channel();
    if (!ec)
        die("rdma_create_event_channel");

    struct rdma_cm_id *conn = NULL;
    if (rdma_create_id(ec, &conn, NULL, RDMA_PS_TCP))
        die("rdma_create_id");

    // Resolve server address (assuming localhost)
    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(PORT));
    server_addr.sin_addr.s_addr = inet_addr("192.168.100.1");

    if (rdma_resolve_addr(conn, NULL, (struct sockaddr *)&server_addr, 2000))
        die("rdma_resolve_addr");

    // Wait for address resolution event
    struct rdma_cm_event *event = NULL;
    if (rdma_get_cm_event(ec, &event))
        die("rdma_get_cm_event");
    rdma_ack_cm_event(event);

    // Resolve route
    if (rdma_resolve_route(conn, 2000))
        die("rdma_resolve_route");

    if (rdma_get_cm_event(ec, &event))
        die("rdma_get_cm_event");
    rdma_ack_cm_event(event);

    // Allocate RDMA resources
    struct rdma_resources res_rdma = {0};
    res_rdma.cm_id = conn;
    res_rdma.pd = ibv_alloc_pd(conn->verbs);
    if (!res_rdma.pd)
        die("ibv_alloc_pd");
    
    res_rdma.cq = ibv_create_cq(conn->verbs, 10, NULL, NULL, 0);
    if (!res_rdma.cq)
        die("ibv_create_cq");

    // Create Queue Pair (QP)
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

    // Allocate and register a send buffer
    res_rdma.send_region = malloc(BUFFER_SIZE);
    if (!res_rdma.send_region)
        die("malloc send_region");
    res_rdma.send_mr = ibv_reg_mr(res_rdma.pd, res_rdma.send_region, BUFFER_SIZE,
                                  IBV_ACCESS_LOCAL_WRITE);
    if (!res_rdma.send_mr)
        die("ibv_reg_mr");

    // Initiate connection (fill in connection parameters as needed)
    struct rdma_conn_param conn_param = {0};
    if (rdma_connect(conn, &conn_param))
        die("rdma_connect");

    // Wait for connection established event
    if (rdma_get_cm_event(ec, &event))
        die("rdma_get_cm_event");
    if (event->event != RDMA_CM_EVENT_ESTABLISHED)
        die("Connection not established");
    rdma_ack_cm_event(event);
    printf("RDMA connection established.\n");

    // --- RDMA Data Transfer ---
    // Example: send an initial message ("Hello") as before
    // snprintf(res_rdma.send_region, BUFFER_SIZE, "Hello");
    struct ibv_sge sge = {
        .addr = (uintptr_t)res_rdma.send_region,
        .length = strlen(res_rdma.send_region) + 1,
        .lkey = res_rdma.send_mr->lkey,
    };
    struct ibv_send_wr send_wr = {0}, *bad_wr = NULL;
    send_wr.sg_list = &sge;
    send_wr.num_sge = 1;
    send_wr.opcode = IBV_WR_SEND;
    send_wr.send_flags = IBV_SEND_SIGNALED;

    // if (ibv_post_send(res_rdma.qp, &send_wr, &bad_wr))
    //     die("ibv_post_send");

    // Normally you would poll the CQ for a completion here.

    // For illustration, you can now send the metadata (row and column counts)
    snprintf(res_rdma.send_region, BUFFER_SIZE, "%llu,%llu", row_count, column_count);
    sge.length = strlen(res_rdma.send_region) + 1;
    if (ibv_post_send(res_rdma.qp, &send_wr, &bad_wr))
        die("ibv_post_send metadata");

    // Then send each row from the DuckDB result...
    // (Loop through the results, fill the send_region, post a send, and wait for completion.)

    // --- Cleanup ---
    rdma_disconnect(conn);
    ibv_dereg_mr(res_rdma.send_mr);
    free(res_rdma.send_region);
    rdma_destroy_qp(conn);
    ibv_destroy_cq(res_rdma.cq);
    ibv_dealloc_pd(res_rdma.pd);
    rdma_destroy_id(conn);
    rdma_destroy_event_channel(ec);
    
    duckdb_destroy_result(&res);
    duckdb_disconnect(&con);
    duckdb_close(&db);

    return 0;
}
