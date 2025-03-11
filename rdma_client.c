#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rdma_client_helper.h"

#define SERVER_IP "192.168.100.1"
#define BUF_SIZE 1024
#define TEST_ITERATIONS 100  // 测试次数

int main(int argc, char **argv) {
    client_context_t client_ctx;
    ucs_status_t status;
    
    // Initialize client
    status = init_client(&client_ctx, SERVER_IP);
    if (status != UCS_OK) {
        fprintf(stderr, "Failed to initialize client\n");
        return 1;
    }
    
    // Connect to server
    status = connect_to_server(&client_ctx);
    if (status != UCS_OK) {
        fprintf(stderr, "Failed to connect to server\n");
        cleanup_client(&client_ctx);
        return 1;
    }
    
    // Run performance test
    run_performance_test(&client_ctx, TEST_ITERATIONS, BUF_SIZE);
    
    // Clean up resources
    cleanup_client(&client_ctx);
    
    return 0;
}
