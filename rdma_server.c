#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "rdma_server_helper.h"

int main(int argc, char **argv) {
    server_context_t server_ctx;
    ucs_status_t status;
    
    // Initialize server
    status = init_server(&server_ctx);
    if (status != UCS_OK) {
        fprintf(stderr, "Failed to initialize server\n");
        return 1;
    }
    
    // Get worker address (optional, for debugging or address publishing)
    ucp_address_t *local_addr;
    size_t address_length;
    status = ucp_worker_get_address(server_ctx.worker, &local_addr, &address_length);
    if (status != UCS_OK) {
        fprintf(stderr, "ucp_worker_get_address failed\n");
        cleanup_server(&server_ctx);
        return 1;
    }
    
    // Create listener
    status = create_server_listener(&server_ctx);
    if (status != UCS_OK) {
        fprintf(stderr, "Failed to create server listener\n");
        ucp_worker_release_address(server_ctx.worker, local_addr);
        cleanup_server(&server_ctx);
        return 1;
    }
    
    // Main event loop
    while (1) {
        ucp_worker_progress(server_ctx.worker);

        // Check all active client connections
        for (int i = 0; i < server_ctx.num_clients; i++) {
            client_connection_t *client = &server_ctx.clients[i];
            if (client->active) {
                // Check if data has been received
                if (client->recv_buffer[0] != '\0') {
                    printf("Received message from client %d: %s\n", i, client->recv_buffer);
                    memset(client->recv_buffer, 0, BUF_SIZE);
                    // Post new receive request
                    post_receive(server_ctx.worker, client);
                }
            }
        }

        usleep(1000);  // Avoid high CPU usage
    }

    // Clean up resources (this part is never reached in this example)
    ucp_worker_release_address(server_ctx.worker, local_addr);
    cleanup_server(&server_ctx);

    return 0;
}
