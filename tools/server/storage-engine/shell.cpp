#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <csignal>
#include "TensorGraph.hpp"

// --- READLINE SUPPORT ---
#if defined(_WIN32) || defined(_WIN64)
    #include <string>
    #include <iostream>
    char* readline(const char* prompt) {
        std::cout << prompt;
        static std::string line;
        if (!std::getline(std::cin, line)) return nullptr;
        char* buf = (char*)malloc(line.length() + 1);
        strcpy(buf, line.c_str());
        return buf;
    }
    void add_history(const char*) {} 
#else
    #include <readline/readline.h>
    #include <readline/history.h>
#endif

// --- ANSI COLORS ---
struct Colors {
    static constexpr const char* RESET   = "\033[0m";
    static constexpr const char* RED     = "\033[31m";
    static constexpr const char* GREEN   = "\033[32m";
    static constexpr const char* YELLOW  = "\033[33m";
    static constexpr const char* BLUE    = "\033[34m";
    static constexpr const char* MAGENTA = "\033[35m";
    static constexpr const char* CYAN    = "\033[36m";
    static constexpr const char* BOLD    = "\033[1m";
    static constexpr const char* DIM     = "\033[2m";
};

// --- SIGNAL HANDLER ---
void sigint_handler(int sig) {
    std::cout << Colors::RESET;
    #if !defined(_WIN32) && !defined(_WIN64)
        std::cout << "\n";
        rl_on_new_line();
        rl_replace_line("", 0);
        rl_redisplay();
    #else
        std::cout << "\n" << Colors::DIM << "(Interrupt) Use 'EXIT' or Ctrl+D to quit." << Colors::RESET << "\nM8> " << std::flush;
    #endif
}

void print_help() {
    std::cout << "\n" << Colors::BOLD << Colors::MAGENTA << "=== M8 Native Storage Engine Help ===" << Colors::RESET << "\n\n";
    std::cout << Colors::YELLOW << "ARCHITECTURE:" << Colors::RESET << "\n";
    std::cout << "  MetaDB -> BigTable -> ColumnGroup -> TensorGraph\n";
    std::cout << "  * " << Colors::CYAN << "BigTable" << Colors::RESET << ": Logical collection of data (e.g., 'Users', 'Products').\n";
    std::cout << "  * " << Colors::CYAN << "ColumnGroup" << Colors::RESET << ": Physical storage engine separating data types (e.g., 'vectors' for AVX access, 'meta' for text).\n";
    std::cout << "  * " << Colors::CYAN << "Column" << Colors::RESET << ": Typed array inside the engine.\n\n";
    
    std::cout << Colors::YELLOW << "COMMANDS:" << Colors::RESET << "\n";
    std::cout << "  " << Colors::GREEN << "CREATE_TABLE" << Colors::RESET << "(name)\n";
    std::cout << "  " << Colors::GREEN << "CREATE_GROUP" << Colors::RESET << "(table, group, size_mb, role)\n";
    std::cout << "  " << Colors::GREEN << "CREATE_COLUMN" << Colors::RESET << "(table, group, name, type, [dim])\n";
    std::cout << "      Types: " << Colors::BOLD << "INT, FLOAT, TEXT, VECTOR" << Colors::RESET << "\n";
    std::cout << "  " << Colors::GREEN << "ADD_ROW" << Colors::RESET << "(table, group, val1...)\n";
    std::cout << "      " << Colors::DIM << "Note: Currently reserves a row. Use UPDATE to set values." << Colors::RESET << "\n";
    std::cout << "  " << Colors::GREEN << "UPDATE" << Colors::RESET << "(table, group, col, row_id, val)\n";
    std::cout << "  " << Colors::GREEN << "GET" << Colors::RESET << "(table, group, col, row_id, type_hint)\n";
    std::cout << "  " << Colors::GREEN << "SELECT" << Colors::RESET << "(table, group, col, limit, offset, type_hint)\n";
    std::cout << "  " << Colors::GREEN << "SEARCH" << Colors::RESET << "(table, group, col, vector, top_k)\n";
    std::cout << "  " << Colors::GREEN << "STATS" << Colors::RESET << "\n";
    std::cout << "  " << Colors::GREEN << "EXIT" << Colors::RESET << "\n\n";

    std::cout << Colors::YELLOW << "EXAMPLES:" << Colors::RESET << "\n";
    std::cout << "  CREATE_TABLE(\"Users\");\n";
    std::cout << "  CREATE_GROUP(\"Users\", \"Bio\", 64, \"data\");\n";
    std::cout << "  CREATE_COLUMN(\"Users\", \"Bio\", \"name\", TEXT);\n";
    std::cout << "  ADD_ROW(\"Users\", \"Bio\");\n";
    std::cout << "  UPDATE(\"Users\", \"Bio\", \"name\", 0, \"Alice\");\n";
    std::cout << "  GET(\"Users\", \"Bio\", \"name\", 0, TEXT);\n";
    std::cout << "  SELECT(\"Users\", \"Bio\", \"name\", 10, 0, TEXT);\n\n";
}


int main() {
    signal(SIGINT, sigint_handler);
    NativeMetaDB db; // The database instance

    std::cout << Colors::BOLD << "M8 Native Storage Engine Shell" << Colors::RESET << "\n";
    std::cout << "Type '" << Colors::CYAN << "HELP" << Colors::RESET << "' for commands or '" << Colors::RED << "EXIT" << Colors::RESET << "' to quit.\n\n";

    std::string prompt_str = std::string(Colors::GREEN) + "M8> " + std::string(Colors::RESET);

    while (true) {
        char* input_raw = readline(prompt_str.c_str());
        if (!input_raw) { std::cout << "\n"; break; } // Ctrl+D

        std::string line(input_raw);
        if (!line.empty()) add_history(input_raw);
        free(input_raw);

        if (line.empty()) continue;
        
        // Handle Shell-specific commands
        if (line == "EXIT" || line == "exit") break;
        if (line == "HELP" || line == "help") { print_help(); continue; }

        // Execute via Query Engine
        std::string result = TGQL::Execute(db, line);
        
        // Output Handling
        if (result.rfind("Error:", 0) == 0) {
            std::cout << Colors::RED << result << Colors::RESET << "\n";
        } else {
            // Check if it's a "Results" block (Search/Select) or simple status
            if (result.find("Results:") != std::string::npos || result.find("Rows ") != std::string::npos) {
                std::cout << Colors::YELLOW << result << Colors::RESET; // Headers are often in result
            } else {
                std::cout << Colors::CYAN << result << Colors::RESET << "\n";
            }
        }
    }

    return 0;
}