# Compiler and flags
CC = gcc
# Add UCX include directory along with your current include paths
CFLAGS = -I./lib -I/usr/local/include

# Add UCX libraries to the linker flags along with your existing ones.
# Note: Make sure the order is correct; if your lib is in ./lib, and UCX is in /usr/local/lib,
# then both -L flags are needed.
LDFLAGS = -L./lib -L/usr/local/lib -lduckdb -Wl,-rpath,./lib -Wl,-rpath,/usr/local/lib -lrdmacm -libverbs -lucp -lucs

# Output directory
BIN_DIR = bin

# Helper library object files
CLIENT_HELPER_OBJ = rdma_client_helper.o
SERVER_HELPER_OBJ = rdma_server_helper.o

# Source files
SOURCES = rdma_client.c rdma_server.c generate_tpch.c

# Targets
TARGETS = $(SOURCES:%.c=$(BIN_DIR)/%)

# Default target
all: $(TARGETS)

# Ensure the output directory exists
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Build client helper object file
$(CLIENT_HELPER_OBJ): rdma_client_helper.c rdma_client_helper.h
	$(CC) $(CFLAGS) -c rdma_client_helper.c -o $(CLIENT_HELPER_OBJ)

# Build server helper object file
$(SERVER_HELPER_OBJ): rdma_server_helper.c rdma_server_helper.h
	$(CC) $(CFLAGS) -c rdma_server_helper.c -o $(SERVER_HELPER_OBJ)

# Build client with helper libraries
$(BIN_DIR)/rdma_client: rdma_client.c $(CLIENT_HELPER_OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) rdma_client.c $(CLIENT_HELPER_OBJ) -o $@ $(LDFLAGS)

# Build server with helper libraries
$(BIN_DIR)/rdma_server: rdma_server.c $(SERVER_HELPER_OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) rdma_server.c $(SERVER_HELPER_OBJ) -o $@ $(LDFLAGS)

# Build generate_tpch
$(BIN_DIR)/generate_tpch: generate_tpch.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# Clean up
clean:
	rm -rf $(BIN_DIR) $(CLIENT_HELPER_OBJ) $(SERVER_HELPER_OBJ)
