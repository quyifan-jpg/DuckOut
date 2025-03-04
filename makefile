# Compiler and flags
CC = gcc
CFLAGS = -I./lib
LDFLAGS = -L./lib -lduckdb -Wl,-rpath,./lib -lrdmacm -libverbs

# Output directory
BIN_DIR = bin

# Source files
SOURCES = server.c client.c generate_tpch.c rdma_client.c rdma_server.c

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
