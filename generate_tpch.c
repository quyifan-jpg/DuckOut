// generate tpch data with scale factor 1

#include "duckdb.h"
#include <stdio.h>

int main() {
    duckdb_connection connection;
    duckdb_database db;
    const char *db_path = "tpch.duckdb";

    // Open the database with the specified path
    if (duckdb_open(db_path, &db) != DuckDBSuccess) {
        fprintf(stderr, "Failed to open database\n");
        return 1;
    }

    // Connect to the database
    if (duckdb_connect(db, &connection) != DuckDBSuccess) {
        fprintf(stderr, "Failed to connect to database\n");
        return 1;
    }

    // Load TPCH extension and generate data with scale factor 1
    duckdb_state state;
    state = duckdb_query(connection, "INSTALL tpch;", NULL);
    state = duckdb_query(connection, "LOAD tpch;", NULL);
    state = duckdb_query(connection, "CALL dbgen(sf=1);", NULL);

    if (state != DuckDBSuccess) {
        fprintf(stderr, "Failed to generate TPCH data\n");
        return 1;
    }

    // Cleanup
    duckdb_disconnect(&connection);
    duckdb_close(&db);

    printf("Successfully generated TPCH data with scale factor 1\n");
    return 0;

}