#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <duckdb.h>

#define PORT 1234

int main(void) {
    duckdb_database db;
    duckdb_connection con;
    duckdb_state status;
    duckdb_result res;
    const char *db_path = "tpch.duckdb";
    
    // --- DuckDB setup ---
    status = duckdb_open(db_path, &db);
    if (status != DuckDBSuccess) {
        fprintf(stderr, "Failed to open DuckDB database\n");
        exit(EXIT_FAILURE);
    }
    status = duckdb_connect(db, &con);
    
    // Execute query to get lineitem data
    status = duckdb_query(con, "SELECT * FROM lineitem", &res);
    if (status != DuckDBSuccess) {
        fprintf(stderr, "Failed to execute query\n");
        exit(EXIT_FAILURE);
    }

    // Get row and column counts from the result
    idx_t row_count = duckdb_row_count(&res);
    idx_t column_count = duckdb_column_count(&res);
    printf("Query returned %llu rows and %llu columns\n", row_count, column_count);

    // --- Socket setup ---
    int sock_fd;
    struct sockaddr_in server_addr;

    // Create UDP socket
    if ((sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);  // Same PORT as server (1234)
    
    // Convert IP address from string to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        exit(EXIT_FAILURE);
    }
    printf("Sending initial message to server\n");
    // Send initial message to server
    const char *hello = "Hello";
    sendto(sock_fd, hello, strlen(hello), 0,
           (struct sockaddr*)&server_addr, sizeof(server_addr));
    
    // Wait for server acknowledgment
    char ack_buffer[256];
    socklen_t server_len = sizeof(server_addr);
    ssize_t n = recvfrom(sock_fd, ack_buffer, sizeof(ack_buffer), 0,
                         (struct sockaddr*)&server_addr, &server_len);
    ack_buffer[n] = '\0';
    printf("Received acknowledgment: %s\n", ack_buffer);

    // Send metadata (remove the duplicate wait for client)
    char metadata_to_send[256];
    snprintf(metadata_to_send, sizeof(metadata_to_send), "%llu,%llu", row_count, column_count);
    sendto(sock_fd, metadata_to_send, strlen(metadata_to_send), 0,
           (struct sockaddr*)&server_addr, sizeof(server_addr));

    // Send each row (update to use server_addr instead of client_addr)
    for (idx_t row = 0; row < row_count; row++) {
        printf("Sending row %llu\n", row);
        char row_data[4096] = {0};  // Adjust buffer size as needed
        int offset = 0;
        
        for (idx_t col = 0; col < column_count; col++) {
            const char* value = duckdb_value_varchar(&res, col, row);
            offset += snprintf(row_data + offset, sizeof(row_data) - offset,
                             "%s%s", col > 0 ? "," : "", value);
            free((void*)value);
        }
        
        sendto(sock_fd, row_data, strlen(row_data), 0,
               (struct sockaddr*)&server_addr, sizeof(server_addr));
    }

    // Clean up
    duckdb_destroy_result(&res);
    close(sock_fd);

    // Clean up DuckDB
    duckdb_disconnect(&con);
    duckdb_close(&db);
    return 0;
}