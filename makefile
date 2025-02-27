# Compiler and flags
CC = gcc
# CFLAGS = -I/opt/homebrew/include
# LDFLAGS = -L/opt/homebrew/lib -lduckdb
CFLAGS = -I/DuckOut/include  # Update include path if needed
LDFLAGS = -L/DuckOut/lib -duckdb 
# Output directory
BIN_DIR = bin

# Targets for server and client
all: $(BIN_DIR)/server $(BIN_DIR)/client $(BIN_DIR)/generate_tpch

# Ensure the output directory exists
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/server: server.c | $(BIN_DIR)
	$(CC) $(CFLAGS) server.c -o $(BIN_DIR)/server $(LDFLAGS)

$(BIN_DIR)/client: client.c | $(BIN_DIR)
	$(CC) $(CFLAGS) client.c -o $(BIN_DIR)/client $(LDFLAGS)

$(BIN_DIR)/generate_tpch: generate_tpch.c | $(BIN_DIR)
	$(CC) $(CFLAGS) generate_tpch.c -o $(BIN_DIR)/generate_tpch $(LDFLAGS)

clean:
	rm -rf $(BIN_DIR)
