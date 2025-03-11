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

# Source files
# If you want to add rdma_uxc.c to your build, add it to the SOURCES list:
SOURCES =   rdma_client.c rdma_server.c generate_tpch.c
# server.c client.c generate_tpch.c rdma_client.c rdma_server.c
# Targets
TARGETS = $(SOURCES:%.c=$(BIN_DIR)/%)

# Default target
all: $(TARGETS)

# Ensure the output directory exists
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Build each target
$(BIN_DIR)/%: %.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# Clean up
clean:
	rm -rf $(BIN_DIR)
