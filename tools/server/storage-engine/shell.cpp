#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <cctype>
#include "TensorGraph.hpp"

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
    std::cout << "\n=== M8 Native Storage Engine Help ===\n\n";
    std::cout << "ARCHITECTURE:\n";
    std::cout << "  MetaDB -> BigTable -> ColumnGroup -> TensorGraph\n";
    std::cout << "  * BigTable: Logical collection of data (e.g., 'Users', 'Products').\n";
    std::cout << "  * ColumnGroup: Physical storage engine separating data types (e.g., 'vectors' for AVX access, 'meta' for text).\n";
    std::cout << "  * Column: Typed array inside the engine.\n\n";
    
    std::cout << "COMMANDS:\n";
    std::cout << "  CREATE_TABLE(name)\n";
    std::cout << "  CREATE_GROUP(table, group, size_mb, role)\n";
    std::cout << "  CREATE_COLUMN(table, group, name, type, [dim])\n";
    std::cout << "      Types: INT, FLOAT, TEXT, VECTOR\n";
    std::cout << "  ADD_ROW(table, group, val1...)\n";
    std::cout << "      Note: Currently reserves a row. Use UPDATE to set values.\n";
    std::cout << "  UPDATE(table, group, col, row_id, val)\n";
    std::cout << "  GET(table, group, col, row_id, type_hint)\n";
    std::cout << "  SEARCH(table, group, col, vector, top_k)\n";
    std::cout << "  STATS\n";
    std::cout << "  EXIT\n\n";

    std::cout << "EXAMPLES:\n";
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
    std::cout << "M8 Native Storage Engine Shell\n";
    std::cout << "Type 'HELP' for commands or 'EXIT' to quit.\n\n";

    std::string line;
    while (true) {
        std::cout << "M8> ";
        if (!std::getline(std::cin, line)) break;
        if (line.empty()) continue;
        
        if (line.back() == ';') line.pop_back();
        
        size_t paren_pos = line.find('(');
        if (paren_pos == std::string::npos) {
            std::string cmd = trim(line);
            std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
            if (cmd == "EXIT") break;
            if (cmd == "STATS") { db.PrintStats(); continue; }
            if (cmd == "HELP") { print_help(); continue; }
            std::cout << "Error: Invalid syntax. Expected COMMAND(args)\n";
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
                std::cout << "Table '" << args[0] << "' created.\n";
            }
            else if (cmd == "CREATE_GROUP") {
                if (args.size() < 4) throw std::runtime_error("CREATE_GROUP requires 4 args: table, group, size_mb, role");
                BigTable* t = db.GetTable(args[0]);
                t->CreateGroup(args[1], std::stoi(args[2]), args[3]);
                std::cout << "Group '" << args[1] << "' created in '" << args[0] << "'.\n";
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
                std::cout << "Column '" << col_name << "' created.\n";
            }
            else if (cmd == "ADD_ROW") {
                if (args.size() < 2) throw std::runtime_error("ADD_ROW requires table, group");
                
                BigTable* t = db.GetTable(args[0]);
                TensorGraph* tg = t->GetGroup(args[1]);
                RowID rid = tg->AddRow();
                
                std::cout << "Row added. ID: " << rid << "\n";
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
                std::cout << "Updated.\n";
            }
            else if (cmd == "SEARCH") {
                if (args.size() < 5) throw std::runtime_error("SEARCH requires: table, group, col, [vec], top_k");
                BigTable* t = db.GetTable(args[0]);
                TensorGraph* tg = t->GetGroup(args[1]);
                
                auto results = tg->VectorSearch(args[2], parse_vector_data(args[3]), std::stoi(args[4]));
                
                std::cout << "Results:\n";
                for(auto& r : results) {
                    std::cout << " - ID: " << r.id << " Score: " << r.score << "\n";
                }
            }
            else if (cmd == "GET") {
                if (args.size() < 5) throw std::runtime_error("GET requires: table, group, col, row_id, TYPE");
                BigTable* t = db.GetTable(args[0]);
                TensorGraph* tg = t->GetGroup(args[1]);
                
                std::string type = args[4];
                std::transform(type.begin(), type.end(), type.begin(), ::toupper);
                
                if (type == "TEXT") std::cout << tg->GetText(args[2], std::stoi(args[3])) << "\n";
                else if (type == "INT") std::cout << tg->GetInt(args[2], std::stoi(args[3])) << "\n";
                else if (type == "FLOAT") std::cout << tg->GetFloat(args[2], std::stoi(args[3])) << "\n";
                else std::cout << "Unknown type hint\n";
            }
            else {
                std::cout << "Unknown command: " << cmd << "\n";
            }

        } catch (const std::exception& ex) {
            std::cerr << "Error: " << ex.what() << "\n";
        }
    }

    return 0;
}