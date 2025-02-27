#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 1234
#define BUFFER_SIZE 4096

int main(void) {
    int sock;
    struct sockaddr_in server_addr, client_addr;
    char buffer[BUFFER_SIZE];
    socklen_t client_len = sizeof(client_addr);
    
    // Create UDP socket
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;  // Listen on all available interfaces
    server_addr.sin_port = htons(PORT);
    
    // Bind socket to server address
    if (bind(sock, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    printf("UDP Server listening on port %d...\n", PORT);
    
    while (1) {  // Add infinite loop to keep server running
        // Receive initial message from client
        ssize_t n = recvfrom(sock, buffer, BUFFER_SIZE, 0, 
                            (struct sockaddr *)&client_addr, &client_len);
        if (n > 0) {
            buffer[n] = '\0';
            printf("Received initial message: %s\n", buffer);
            
            // Send acknowledgment back to client
            const char *ack_msg = "ready";
            sendto(sock, ack_msg, strlen(ack_msg), 0, 
                   (struct sockaddr *)&client_addr, client_len);

            // Receive metadata (row count and column count)
            n = recvfrom(sock, buffer, BUFFER_SIZE, 0, 
                        (struct sockaddr *)&client_addr, &client_len);
            buffer[n] = '\0';
            
            long row_count, col_count;
            sscanf(buffer, "%ld,%ld", &row_count, &col_count);
            printf("Expected rows: %ld, columns: %ld\n", row_count, col_count);
            
            // Receive and count rows
            long received_rows = 0;
            while (received_rows < row_count) {
                n = recvfrom(sock, buffer, BUFFER_SIZE, 0, 
                            (struct sockaddr *)&client_addr, &client_len);
                if (n > 0) {
                    buffer[n] = '\0';
                    received_rows++;
                    if (received_rows % 10000 == 0) {
                        printf("Received %ld rows...\n", received_rows);
                    }
                }
            }
            
            printf("Total rows received: %ld\n", received_rows);
        }
    }
    
    // This code will never be reached unless you add a break condition
    close(sock);
    return 0;
}