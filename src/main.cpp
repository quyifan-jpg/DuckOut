#include "duckdb.hpp"
#include <iostream>

int main() {
    try {
        // Create an in-memory DuckDB database
        duckdb::DuckDB db(nullptr);
        duckdb::Connection con(db);

        // Execute a simple query
        con.Query("CREATE TABLE users (id INTEGER, name VARCHAR);");
        con.Query("INSERT INTO users VALUES (1, 'Alice'), (2, 'Bob');");

        auto result = con.Query("SELECT * FROM users;");
        result->Print(); // Prints the results

    } catch (std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}