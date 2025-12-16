#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <cctype>
#include "TensorGraph.hpp"

// --- READLINE SUPPORT ---
// Use GNU Readline or libedit for proper input handling (arrow keys, history)
// Compile with: g++ main.cpp -o m8shell -O3 -lreadline
#if defined(_WIN32) || defined(_WIN64)
    #include <string>
    #include <iostream>
    // Windows fallback: Simple getline (no history/arrows without external lib)
    char* readline(const char* prompt) {
        std::cout << prompt;
        static std::string line;
        if (!std::getline(std::cin, line)) return nullptr;
        // Duplicate string for compatibility with free()
        char* buf = (char*)malloc(line.length() + 1);
        strcpy(buf, line.c_str());
        return buf;
    }
    void add_history(const char*) {} // No-op
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

// --- SHELL UTILITIES ---

std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(' ');
    if (std::string::npos == first) return str;
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
}

std::vector<std::string> parse_arguments(const std::string& args_str) {
    std::vector<std::string> args;
    std::string current;
    bool in_quote = false;
    bool in_bracket = false;

    for (char c : args_str) {
        if (c == '"') {
            in_quote = !in_quote;
        } else if (c == '[' && !in_quote) {
            in_bracket = true;
            current += c;
        } else if (c == ']' && !in_quote) {
            in_bracket = false;
            current += c;
        } else if (c == ',' && !in_quote && !in_bracket) {
            args.push_back(trim(current));
            current.clear();
        } else {
            current += c;
        }
    }
    if (!current.empty()) args.push_back(trim(current));
    return args;
}

ColType parse_type(std::string type_str, int& out_dim) {
    std::transform(type_str.begin(), type_str.end(), type_str.begin(), ::toupper);
    if (type_str == "INT" || type_str == "INTEGER") return ColType::INT32;
    if (type_str == "FLOAT") return ColType::FLOAT32;
    if (type_str == "TEXT" || type_str == "STRING") return ColType::TEXT;
    
    if (type_str.find("VECTOR") == 0) {
        return ColType::VECTOR_F32;
    }
    throw std::runtime_error("Unknown type: " + type_str);
}

std::vector<float> parse_vector_data(std::string val) {
    if (val.front() == '[') val = val.substr(1);
    if (val.back() == ']') val.pop_back();
    std::vector<float> vec;
    std::stringstream ss(val);
    std::string item;
    while (std::getline(ss, item, ',')) {
        vec.push_back(std::stof(item));
    }
    return vec;
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
    std::cout << "  " << Colors::GREEN << "SEARCH" << Colors::RESET << "(table, group, col, vector, top_k)\n";
    std::cout << "  " << Colors::GREEN << "STATS" << Colors::RESET << "\n";
    std::cout << "  " << Colors::GREEN << "EXIT" << Colors::RESET << "\n\n";

    std::cout << Colors::YELLOW << "EXAMPLES:" << Colors::RESET << "\n";
    std::cout << "  CREATE_TABLE(\"Users\");\n";
    std::cout << "  CREATE_GROUP(\"Users\", \"Bio\", 64, \"data\");\n";
    std::cout << "  CREATE_COLUMN(\"Users\", \"Bio\", \"name\", TEXT);\n";
    std::cout << "  ADD_ROW(\"Users\", \"Bio\");\n";
    std::cout << "  UPDATE(\"Users\", \"Bio\", \"name\", 0, \"Alice\");\n";
    std::cout << "  GET(\"Users\", \"Bio\", \"name\", 0, TEXT);\n\n";
}

// --- MAIN SHELL ---

int main() {
    NativeMetaDB db;
    std::cout << Colors::BOLD << "M8 Native Storage Engine Shell" << Colors::RESET << "\n";
    std::cout << "Type '" << Colors::CYAN << "HELP" << Colors::RESET << "' for commands or '" << Colors::RED << "EXIT" << Colors::RESET << "' to quit.\n\n";

    // Setup color prompt string
    std::string prompt_str = std::string(Colors::GREEN) + "M8> " + std::string(Colors::RESET);

    while (true) {
        // Use readline for input
        char* input_raw = readline(prompt_str.c_str());
        
        // Handle EOF (Ctrl+D)
        if (!input_raw) {
            std::cout << "\n";
            break;
        }

        std::string line(input_raw);
        
        // Add non-empty lines to history
        if (!line.empty()) {
            add_history(input_raw);
        }
        
        // Free the memory allocated by readline
        free(input_raw);

        if (line.empty()) continue;
        
        if (line.back() == ';') line.pop_back();
        
        size_t paren_pos = line.find('(');
        if (paren_pos == std::string::npos) {
            std::string cmd = trim(line);
            std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
            if (cmd == "EXIT") break;
            if (cmd == "STATS") { db.PrintStats(); continue; }
            if (cmd == "HELP") { print_help(); continue; }
            std::cout << Colors::RED << "Error: Invalid syntax. Expected COMMAND(args)" << Colors::RESET << "\n";
            continue;
        }

        std::string cmd = trim(line.substr(0, paren_pos));
        std::string args_content = line.substr(paren_pos + 1);
        if (args_content.back() == ')') args_content.pop_back(); 

        std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
        std::vector<std::string> args = parse_arguments(args_content);

        try {
            if (cmd == "CREATE_TABLE") {
                if (args.empty()) throw std::runtime_error("CREATE_TABLE requires 1 arg: name");
                db.CreateTable(args[0]);
                std::cout << "Table '" << Colors::CYAN << args[0] << Colors::RESET << "' created.\n";
            }
            else if (cmd == "CREATE_GROUP") {
                if (args.size() < 4) throw std::runtime_error("CREATE_GROUP requires 4 args: table, group, size_mb, role");
                BigTable* t = db.GetTable(args[0]);
                t->CreateGroup(args[1], std::stoi(args[2]), args[3]);
                std::cout << "Group '" << Colors::CYAN << args[1] << Colors::RESET << "' created in '" << args[0] << "'.\n";
            }
            else if (cmd == "CREATE_COLUMN") {
                if (args.size() < 4) throw std::runtime_error("CREATE_COLUMN requires table, group, name, type");
                
                BigTable* t = db.GetTable(args[0]);
                TensorGraph* tg = t->GetGroup(args[1]);
                std::string col_name = args[2];
                
                int dim = 0;
                ColType type = parse_type(args[3], dim);
                
                if (type == ColType::VECTOR_F32) {
                    if (args.size() >= 5) dim = std::stoi(args[4]);
                    else throw std::runtime_error("VECTOR type requires 5th argument: dimension");
                }

                tg->CreateColumn(col_name, type, 1000, dim); 
                std::cout << "Column '" << Colors::CYAN << col_name << Colors::RESET << "' created.\n";
            }
            else if (cmd == "ADD_ROW") {
                if (args.size() < 2) throw std::runtime_error("ADD_ROW requires table, group");
                
                BigTable* t = db.GetTable(args[0]);
                TensorGraph* tg = t->GetGroup(args[1]);
                RowID rid = tg->AddRow();
                
                std::cout << "Row added. ID: " << Colors::YELLOW << rid << Colors::RESET << "\n";
            }
            else if (cmd == "UPDATE" || cmd == "SET") {
                if (args.size() < 5) throw std::runtime_error("UPDATE requires: table, group, col, row_id, val");
                BigTable* t = db.GetTable(args[0]);
                TensorGraph* tg = t->GetGroup(args[1]);
                std::string col = args[2];
                RowID rid = std::stoi(args[3]);
                std::string val = args[4];

                if (val.find('[') != std::string::npos) {
                    tg->SetVector(col, rid, parse_vector_data(val));
                } else if (val.find_first_not_of("0123456789-") == std::string::npos) {
                    tg->SetInt(col, rid, std::stoi(val));
                } else if (val.find('.') != std::string::npos) {
                    try { tg->SetFloat(col, rid, std::stof(val)); } 
                    catch(...) { tg->SetText(col, rid, val); } 
                } else {
                    tg->SetText(col, rid, val);
                }
                std::cout << Colors::GREEN << "Updated." << Colors::RESET << "\n";
            }
            else if (cmd == "SEARCH") {
                if (args.size() < 5) throw std::runtime_error("SEARCH requires: table, group, col, [vec], top_k");
                BigTable* t = db.GetTable(args[0]);
                TensorGraph* tg = t->GetGroup(args[1]);
                
                auto results = tg->VectorSearch(args[2], parse_vector_data(args[3]), std::stoi(args[4]));
                
                std::cout << Colors::YELLOW << "Results:" << Colors::RESET << "\n";
                for(auto& r : results) {
                    std::cout << " - ID: " << Colors::CYAN << r.id << Colors::RESET << " Score: " << Colors::GREEN << r.score << Colors::RESET << "\n";
                }
            }
            else if (cmd == "GET") {
                if (args.size() < 5) throw std::runtime_error("GET requires: table, group, col, row_id, TYPE");
                BigTable* t = db.GetTable(args[0]);
                TensorGraph* tg = t->GetGroup(args[1]);
                
                std::string type = args[4];
                std::transform(type.begin(), type.end(), type.begin(), ::toupper);
                
                std::cout << Colors::CYAN << "< " << Colors::RESET;
                if (type == "TEXT") std::cout << tg->GetText(args[2], std::stoi(args[3])) << "\n";
                else if (type == "INT") std::cout << tg->GetInt(args[2], std::stoi(args[3])) << "\n";
                else if (type == "FLOAT") std::cout << tg->GetFloat(args[2], std::stoi(args[3])) << "\n";
                else std::cout << Colors::RED << "Unknown type hint" << Colors::RESET << "\n";
            }
            else {
                std::cout << Colors::RED << "Unknown command: " << cmd << Colors::RESET << "\n";
            }

        } catch (const std::exception& ex) {
            std::cerr << Colors::RED << "Error: " << ex.what() << Colors::RESET << "\n";
        }
    }

    return 0;
}