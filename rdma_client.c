// rdma_client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <rdma/rdma_cma.h>
#include <infiniband/verbs.h>
#include <duckdb.h>

#define PORT "12345"  // Use a string port for rdma_resolve_addr
#define BUFFER_SIZE (1024 * 1024)  // 1MB buffer
#define NUM_BUFFERS 4              // Use multiple buffers for pipelining

// Structure to hold RDMA resources
struct rdma_resources {
    struct rdma_cm_id *cm_id;
    struct ibv_pd *pd;
    struct ibv_cq *cq;
    struct ibv_qp *qp;
    char *send_regions[NUM_BUFFERS];
    struct ibv_mr *send_mrs[NUM_BUFFERS];
    int current_buffer;
};

// Add this function before main()
static char* get_value_as_string(duckdb_result *result, idx_t row, idx_t col) {
    // First check if the value is NULL
    if (duckdb_value_is_null(result, col, row)) {
        return strdup("NULL");  // Return "NULL" for NULL values
    }

    // Get column type
    duckdb_type type = duckdb_column_type(result, col);
    char buffer[256];  // Temporary buffer for number conversions

    switch (type) {
        case DUCKDB_TYPE_BIGINT: {
            // For l_orderkey, l_partkey, l_suppkey
            int64_t val = duckdb_value_int64(result, col, row);
            snprintf(buffer, sizeof(buffer), "%ld", val);
            return strdup(buffer);
        }
        case DUCKDB_TYPE_DECIMAL: {
            // For l_quantity, l_extendedprice, l_discount, l_tax
            double val = duckdb_value_double(result, col, row);
            snprintf(buffer, sizeof(buffer), "%.2f", val);
            return strdup(buffer);
        }
        case DUCKDB_TYPE_DATE: {
            // For l_shipdate, l_commitdate, l_receiptdate
            const char* date_str = duckdb_value_varchar(result, col, row);
            char* result = strdup(date_str);
            duckdb_free((void*)date_str);
            return result;
        }
        case DUCKDB_TYPE_VARCHAR: {
            // For l_shipmode, l_comment
            const char* str_val = duckdb_value_varchar(result, col, row);
            if (!str_val || str_val[0] == '\0') {
                return strdup("EMPTY");
            }
            return (char*)str_val;  // Already allocated by DuckDB
        }
        default: {
            // Fallback for any other types
            const char* str_val = duckdb_value_varchar(result, col, row);
            if (!str_val) {
                return strdup("NULL");
            }
            return (char*)str_val;
        }
    }
}

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

    // Print the query we're executing


    status = duckdb_query(con, "SELECT * FROM lineitem LIMIT 1000", &res);
    if (status != DuckDBSuccess) {
        fprintf(stderr, "Failed to execute query\n");
        exit(EXIT_FAILURE);
    }
    idx_t row_count = duckdb_row_count(&res);
    idx_t column_count = duckdb_column_count(&res);
    printf("Query returned %llu rows and %llu columns\n\n", row_count, column_count);

    // Print column names
    printf("Column names:\n");
    for (idx_t col = 0; col < column_count; col++) {
        const char* col_name = duckdb_column_name(&res, col);
        printf("  Column %llu: %s\n", col, col_name);
    }
    printf("\n----------------------------------------\n");

    // Print first 10 rows
    // printf("First 10 rows of data:\n");
    // for (idx_t row = 0; row < (row_count < 10 ? row_count : 10); row++) {
    //     printf("\nRow %llu:\n", row);
    //     for (idx_t col = 0; col < column_count; col++) {
    //         char* str_val = get_value_as_string(&res, row, col);
    //         printf("  Column %llu: %s\n", col, str_val);
    //         duckdb_free(str_val);
    //     }
    //     printf("----------------------------------------\n");
    // }

    // --- RDMA Setup ---
    printf("Creating RDMA event channel...\n");
    struct rdma_event_channel *ec = rdma_create_event_channel();
    if (!ec)
        die("rdma_create_event_channel");
    printf("Event channel created successfully\n");

    printf("Creating RDMA ID...\n");
    struct rdma_cm_id *conn = NULL;
    if (rdma_create_id(ec, &conn, NULL, RDMA_PS_TCP))
        die("rdma_create_id");
    printf("RDMA ID created successfully\n");

    // Resolve server address
    printf("Resolving address for server %s:%s...\n", "192.168.100.1", PORT);
    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(PORT));
    server_addr.sin_addr.s_addr = inet_addr("192.168.100.1");

    if (rdma_resolve_addr(conn, NULL, (struct sockaddr *)&server_addr, 2000))
        die("rdma_resolve_addr");
    printf("Address resolution initiated\n");

    // Wait for address resolution event
    printf("Waiting for address resolution event...\n");
    struct rdma_cm_event *event = NULL;
    if (rdma_get_cm_event(ec, &event))
        die("rdma_get_cm_event");
    printf("Address resolved successfully\n");
    rdma_ack_cm_event(event);

    // Resolve route
    printf("Resolving route...\n");
    if (rdma_resolve_route(conn, 2000))
        die("rdma_resolve_route");

    printf("Waiting for route resolution event...\n");
    if (rdma_get_cm_event(ec, &event))
        die("rdma_get_cm_event");
    printf("Route resolved successfully\n");
    rdma_ack_cm_event(event);

    // Allocate RDMA resources
    printf("Allocating Protection Domain (PD)...\n");
    struct rdma_resources res_rdma = {0};
    res_rdma.cm_id = conn;
    res_rdma.pd = ibv_alloc_pd(conn->verbs);
    if (!res_rdma.pd)
        die("ibv_alloc_pd");
    printf("PD allocated successfully\n");
    
    printf("Creating Completion Queue (CQ)...\n");
    res_rdma.cq = ibv_create_cq(conn->verbs, 32, NULL, NULL, 0);
    if (!res_rdma.cq)
        die("ibv_create_cq");
    printf("CQ created successfully with %d entries\n", res_rdma.cq->cqe);

    // Create Queue Pair (QP)
    printf("Creating Queue Pair (QP)...\n");
    struct ibv_qp_init_attr qp_attr = {0};
    qp_attr.send_cq = res_rdma.cq;
    qp_attr.recv_cq = res_rdma.cq;
    qp_attr.qp_type = IBV_QPT_RC;    // Reliable Connection
    qp_attr.cap.max_send_wr = 32;    // Increased for better performance
    qp_attr.cap.max_recv_wr = 32;    // Increased for better performance
    qp_attr.cap.max_send_sge = 1;    // Single scatter-gather element
    qp_attr.cap.max_recv_sge = 1;    // Single scatter-gather element
    qp_attr.cap.max_inline_data = 64; // Enable inline data for small messages

    if (rdma_create_qp(conn, res_rdma.pd, &qp_attr))
        die("rdma_create_qp");
    res_rdma.qp = conn->qp;
    printf("QP created successfully (QP num: %u)\n", res_rdma.qp->qp_num);

    // Allocate multiple send buffers
    printf("Allocating and registering %d send buffers of size %d bytes each...\n", 
           NUM_BUFFERS, BUFFER_SIZE);
    for (int i = 0; i < NUM_BUFFERS; i++) {
        res_rdma.send_regions[i] = malloc(BUFFER_SIZE);
        if (!res_rdma.send_regions[i])
            die("malloc send_region");
        res_rdma.send_mrs[i] = ibv_reg_mr(res_rdma.pd, res_rdma.send_regions[i], 
            BUFFER_SIZE, IBV_ACCESS_LOCAL_WRITE);
        if (!res_rdma.send_mrs[i])
            die("ibv_reg_mr");
        printf("Buffer %d registered successfully (lkey: %u)\n", i, res_rdma.send_mrs[i]->lkey);
    }

    // Initiate connection
    printf("Initiating connection to server...\n");
    struct rdma_conn_param conn_param = {0};
    conn_param.responder_resources = 1;
    conn_param.initiator_depth = 1;
    conn_param.retry_count = 7;    // 3 bits field
    conn_param.rnr_retry_count = 7; // 3 bits field
    
    if (rdma_connect(conn, &conn_param))
        die("rdma_connect");

    // Wait for connection established event
    printf("Waiting for connection establishment...\n");
    if (rdma_get_cm_event(ec, &event))
        die("rdma_get_cm_event");
    if (event->event != RDMA_CM_EVENT_ESTABLISHED)
        die("Connection not established");
    printf("RDMA connection established successfully!\n");
    printf("Local QP number: %u\n", res_rdma.qp->qp_num);
    printf("Remote QP number: %u\n", event->param.conn.qp_num);
    rdma_ack_cm_event(event);

    // Send metadata first
    snprintf(res_rdma.send_regions[0], BUFFER_SIZE, "%llu,%llu", row_count, column_count);
    struct ibv_sge sge = {
        .addr = (uintptr_t)res_rdma.send_regions[0],
        .length = strlen(res_rdma.send_regions[0]) + 1,
        .lkey = res_rdma.send_mrs[0]->lkey,
    };
    struct ibv_send_wr send_wr = {0}, *bad_wr = NULL;
    send_wr.sg_list = &sge;
    send_wr.num_sge = 1;
    send_wr.opcode = IBV_WR_SEND;
    send_wr.send_flags = IBV_SEND_SIGNALED;

    if (ibv_post_send(res_rdma.qp, &send_wr, &bad_wr))
        die("ibv_post_send metadata");

    // Wait for completion
    struct ibv_wc wc;
    while (ibv_poll_cq(res_rdma.cq, 1, &wc) < 1)
        ;
    if (wc.status != IBV_WC_SUCCESS)
        die("Failed send completion");

    // Send DuckDB data in chunks
    res_rdma.current_buffer = 0;
    for (idx_t row = 0; row < row_count; row++) {
        char *buf = res_rdma.send_regions[res_rdma.current_buffer];
        // Clear the buffer before use
        memset(buf, 0, BUFFER_SIZE);
        size_t offset = 0;
        
        printf("Sending Row %llu\n", row);
        // Serialize row data into buffer
        for (idx_t col = 0; col < column_count; col++) {
            // Convert value to string regardless of type
            char* str_val = get_value_as_string(&res, row, col);
            
            // Write to buffer
            int written = snprintf(buf + offset, BUFFER_SIZE - offset, 
                                 "%s%s", str_val, 
                                 (col < column_count - 1) ? "," : "\n");
            offset += written;
            
            // Free the string
            duckdb_free(str_val);
        }

        // Post send for this buffer
        sge.addr = (uintptr_t)buf;
        sge.length = offset;
        sge.lkey = res_rdma.send_mrs[res_rdma.current_buffer]->lkey;
        
        // Add debug prints
        printf("Posting send with:\n");
        printf("  Buffer address: %p\n", (void*)sge.addr);
        printf("  Length: %u\n", sge.length);
        printf("  LKey: %u\n", sge.lkey);
        printf("  QP num: %u\n", res_rdma.qp->qp_num);

        if (ibv_post_send(res_rdma.qp, &send_wr, &bad_wr))
            die("ibv_post_send row");

        // Wait for server's acknowledgment
        struct ibv_recv_wr recv_wr = {0}, *bad_recv_wr = NULL;
        struct ibv_sge recv_sge = {
            .addr = (uintptr_t)res_rdma.send_regions[res_rdma.current_buffer],
            .length = BUFFER_SIZE,
            .lkey = res_rdma.send_mrs[res_rdma.current_buffer]->lkey
        };
        recv_wr.sg_list = &recv_sge;
        recv_wr.num_sge = 1;

        // Post receive for acknowledgment
        if (ibv_post_recv(res_rdma.qp, &recv_wr, &bad_recv_wr))
            die("ibv_post_recv ack");

        // Wait for acknowledgment completion
        while (ibv_poll_cq(res_rdma.cq, 1, &wc) < 1)
            ;
        if (wc.status != IBV_WC_SUCCESS)
            die("Failed receive completion for ack");

        printf("Received acknowledgment for row %llu\n", row);

        res_rdma.current_buffer = (res_rdma.current_buffer + 1) % NUM_BUFFERS;
    }

    // --- Cleanup ---
    rdma_disconnect(conn);
    for (int i = 0; i < NUM_BUFFERS; i++) {
        ibv_dereg_mr(res_rdma.send_mrs[i]);
        free(res_rdma.send_regions[i]);
    }
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
