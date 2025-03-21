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
SOURCES = generate_tpch.c ucp_hello_world.c ucp_example_server.c

# Targets
TARGETS = $(SOURCES:%.c=$(BIN_DIR)/%)

# Default target
all: $(TARGETS)

# Ensure the output directory exists
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Build generate_tpch
$(BIN_DIR)/generate_tpch: generate_tpch.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# Build ucp_hello_world
$(BIN_DIR)/ucp_hello_world: ucp_hello_world.c | $(BIN_DIR)
	$(CC) $(CFLAGS) ucp_hello_world.c -o $@ $(LDFLAGS)

# Clean up
clean:
	rm -rf $(BIN_DIR)
