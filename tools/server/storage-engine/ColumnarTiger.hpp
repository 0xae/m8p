#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdint>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <iomanip>
#include <unordered_map>
#include <memory>
#include <sstream>
#include <fstream>

// Check for AVX support via compiler flags
#if defined(__AVX2__) || defined(__AVX512F__)
#include <immintrin.h>
#endif

// --- Configuration & Constants ---
constexpr size_t ALIGNMENT_BYTES = 64; 
constexpr uint32_t DELETED_FLAG = 0xFFFFFFFF; 

// --- Types ---
using RelPtr = uint32_t;
using RowID = uint32_t;

enum class ColType : uint8_t {
    INT32,
    FLOAT32,
    TEXT,
    VECTOR_F32
};

struct ColumnHeader {
    char name[32];
    ColType type;
    uint32_t count;
    uint32_t capacity;
    uint16_t vector_dim; 
    RelPtr data_offset;
};

// --- SIMD Helpers ---
// Force target attributes to ensure FMA instructions are available
#if defined(__GNUC__) || defined(__clang__)
    #if defined(__AVX512F__)
    __attribute__((target("avx512f")))
    #elif defined(__AVX2__)
    __attribute__((target("avx2,fma")))
    #endif
#endif
inline float l2_sq_simd(const float* a, const float* b, int dim) {
#if defined(__AVX512F__)
    __m512 sum = _mm512_setzero_ps();
    int i = 0;
    for (; i <= dim - 16; i += 16) {
        __m512 v1 = _mm512_loadu_ps(a + i);
        __m512 v2 = _mm512_loadu_ps(b + i);
        __m512 diff = _mm512_sub_ps(v1, v2);
        sum = _mm512_fmadd_ps(diff, diff, sum);
    }
    float total = _mm512_reduce_add_ps(sum);
    for (; i < dim; ++i) {
        float diff = a[i] - b[i];
        total += diff * diff;
    }
    return total;
#elif defined(__AVX2__)
    __m256 sum = _mm256_setzero_ps();
    int i = 0;
    for (; i <= dim - 8; i += 8) {
        __m256 v1 = _mm256_loadu_ps(a + i);
        __m256 v2 = _mm256_loadu_ps(b + i);
        __m256 diff = _mm256_sub_ps(v1, v2);
        sum = _mm256_fmadd_ps(diff, diff, sum); 
    }
    float result[8];
    _mm256_storeu_ps(result, sum);
    float total = result[0] + result[1] + result[2] + result[3] + 
                  result[4] + result[5] + result[6] + result[7];
    for (; i < dim; ++i) {
        float diff = a[i] - b[i];
        total += diff * diff;
    }
    return total;
#else
    float sum = 0.0f;
    for(int i=0; i<dim; ++i) {
        float diff = a[i] - b[i];
        sum += diff * diff;
    }
    return sum;
#endif
}

// --- ColumnarTiger Engine (Formerly TensorGraph) ---
class ColumnarTiger {
private:
    uint8_t* memory_block;
    size_t capacity_bytes;
    size_t head; 
    
    std::unordered_map<std::string, int> column_map; 
    std::vector<ColumnHeader> columns;

    inline size_t align_forward(size_t ptr, size_t align) {
        return (ptr + align - 1) & ~(align - 1);
    }

    RelPtr allocate(size_t size_bytes, size_t manual_align = 0) {
        size_t align = (manual_align > 0) ? manual_align : ALIGNMENT_BYTES;
        size_t aligned_offset = align_forward(head, align);
        if (aligned_offset + size_bytes > capacity_bytes) throw std::runtime_error("OOM");
        head = aligned_offset + size_bytes;
        return (RelPtr)aligned_offset;
    }

    template <typename T> T* get_ptr(RelPtr offset) {
        return reinterpret_cast<T*>(memory_block + offset);
    }

    template <typename T> T* get_cell_ptr(int col_idx, RowID row) {
        if (col_idx < 0 || col_idx >= columns.size()) throw std::runtime_error("Invalid column");
        ColumnHeader& col = columns[col_idx];
        if (row >= col.capacity) throw std::runtime_error("Row index out of bounds");
        return get_ptr<T>(col.data_offset) + row;
    }

    // --- String Helpers ---
    static std::string str_to_upper(const std::string& s) {
        std::string data = s;
        std::transform(data.begin(), data.end(), data.begin(), ::toupper);
        return data;
    }
    static std::string str_to_lower(const std::string& s) {
        std::string data = s;
        std::transform(data.begin(), data.end(), data.begin(), ::tolower);
        return data;
    }
    static bool str_contains(const std::string& haystack, const std::string& needle) {
        return haystack.find(needle) != std::string::npos;
    }
    static bool str_starts_with(const std::string& str, const std::string& prefix) {
        return str.rfind(prefix, 0) == 0;
    }
    static bool str_ends_with(const std::string& str, const std::string& suffix) {
        if (str.length() < suffix.length()) return false;
        return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
    }

public:
    ColumnarTiger(size_t size_mb) {
        capacity_bytes = size_mb * 1024 * 1024;
        memory_block = new uint8_t[capacity_bytes]; 
        std::memset(memory_block, 0, capacity_bytes);
        head = 0; 
    }

    ~ColumnarTiger() { delete[] memory_block; }

    void Reset() {
        head = 0;
        columns.clear();
        column_map.clear();
    }

    void Save(const std::string& filepath) {
        std::ofstream out(filepath, std::ios::binary);
        if (!out) throw std::runtime_error("Cannot open file for writing: " + filepath);
        out.write(reinterpret_cast<const char*>(&capacity_bytes), sizeof(size_t));
        out.write(reinterpret_cast<const char*>(&head), sizeof(size_t));
        size_t num_cols = columns.size();
        out.write(reinterpret_cast<const char*>(&num_cols), sizeof(size_t));
        if (num_cols > 0) {
            out.write(reinterpret_cast<const char*>(columns.data()), num_cols * sizeof(ColumnHeader));
        }
        out.write(reinterpret_cast<const char*>(memory_block), head);
        out.close();
    }

    static std::unique_ptr<ColumnarTiger> Load(const std::string& filepath) {
        std::ifstream in(filepath, std::ios::binary);
        if (!in) throw std::runtime_error("Cannot open file for reading: " + filepath);
        size_t cap, hd;
        in.read(reinterpret_cast<char*>(&cap), sizeof(size_t));
        in.read(reinterpret_cast<char*>(&hd), sizeof(size_t));
        auto db = std::make_unique<ColumnarTiger>(cap / (1024 * 1024)); 
        db->head = hd;
        size_t num_cols;
        in.read(reinterpret_cast<char*>(&num_cols), sizeof(size_t));
        db->columns.resize(num_cols);
        if (num_cols > 0) {
            in.read(reinterpret_cast<char*>(db->columns.data()), num_cols * sizeof(ColumnHeader));
        }
        for (int i = 0; i < num_cols; ++i) {
            db->column_map[db->columns[i].name] = i;
        }
        in.read(reinterpret_cast<char*>(db->memory_block), hd);
        in.close();
        return db;
    }

    void CreateColumn(const std::string& name, ColType type, uint32_t initial_capacity, int dim = 0) {
        if (column_map.find(name) != column_map.end()) throw std::runtime_error("Column exists");
        ColumnHeader header;
        std::strncpy(header.name, name.c_str(), 31);
        header.name[31] = '\0';
        header.type = type;
        header.count = 0;
        header.capacity = initial_capacity;
        header.vector_dim = (type == ColType::VECTOR_F32) ? dim : 0;

        size_t element_size = 0;
        size_t alignment = 0; 
        switch(type) {
            case ColType::INT32: element_size = sizeof(int32_t); alignment = 4; break;
            case ColType::FLOAT32: element_size = sizeof(float); alignment = 4; break;
            case ColType::TEXT: element_size = sizeof(RelPtr); alignment = 4; break;
            case ColType::VECTOR_F32: element_size = sizeof(float) * dim; alignment = 64; break;
        }
        header.data_offset = allocate(element_size * initial_capacity, alignment);
        columns.push_back(header);
        column_map[name] = columns.size() - 1;
    }

    RowID AddRow() {
        if (columns.empty()) return 0;
        uint32_t current_count = columns[0].count;
        if (current_count >= columns[0].capacity) throw std::runtime_error("Capacity reached");
        for (auto& col : columns) col.count++;
        return current_count; 
    }

    void SetInt(const std::string& col_name, RowID row, int32_t val) {
        *get_cell_ptr<int32_t>(column_map.at(col_name), row) = val;
    }
    void SetFloat(const std::string& col_name, RowID row, float val) {
        *get_cell_ptr<float>(column_map.at(col_name), row) = val;
    }
    void SetVector(const std::string& col_name, RowID row, const std::vector<float>& vec) {
        int idx = column_map.at(col_name);
        ColumnHeader& col = columns[idx];
        if(col.type != ColType::VECTOR_F32 || vec.size() != col.vector_dim) throw std::runtime_error("Type/Dim mismatch");
        float* dest = get_ptr<float>(col.data_offset) + (row * col.vector_dim);
        std::memcpy(dest, vec.data(), vec.size() * sizeof(float));
    }
    void SetText(const std::string& col_name, RowID row, const std::string& text) {
        int idx = column_map.at(col_name);
        RelPtr str_offset = allocate(text.size() + 1, 1);
        std::memcpy(get_ptr<char>(str_offset), text.c_str(), text.size() + 1);
        *get_cell_ptr<RelPtr>(idx, row) = str_offset;
    }

    int32_t GetInt(const std::string& col_name, RowID row) {
        return *get_cell_ptr<int32_t>(column_map.at(col_name), row);
    }
    float GetFloat(const std::string& col_name, RowID row) {
        return *get_cell_ptr<float>(column_map.at(col_name), row);
    }
    std::string GetText(const std::string& col_name, RowID row) {
        RelPtr offset = *get_cell_ptr<RelPtr>(column_map.at(col_name), row);
        if (offset == DELETED_FLAG) return "";
        return std::string(get_ptr<char>(offset));
    }
    std::vector<float> GetVector(const std::string& col_name, RowID row) {
        int idx = column_map.at(col_name);
        ColumnHeader& col = columns[idx];
        float* src = get_ptr<float>(col.data_offset) + (row * col.vector_dim);
        return std::vector<float>(src, src + col.vector_dim);
    }
    
    // Check if column exists and get type
    bool HasColumn(const std::string& name) { return column_map.find(name) != column_map.end(); }
    ColType GetColumnType(const std::string& name) { return columns[column_map.at(name)].type; }

    uint32_t GetRowCount(const std::string& col_name) {
        if (column_map.find(col_name) == column_map.end()) throw std::runtime_error("Column not found: " + col_name);
        return columns[column_map.at(col_name)].count;
    }

    struct SearchResult { RowID id; float score; };

    std::vector<SearchResult> VectorSearch(const std::string& col_name, const std::vector<float>& query, int top_k) {
        int idx = column_map.at(col_name);
        ColumnHeader& col = columns[idx];
        if (query.size() != col.vector_dim) throw std::runtime_error("Query dim mismatch");
        std::vector<SearchResult> results;
        results.reserve(col.count);
        float* data_base = get_ptr<float>(col.data_offset);
        for(uint32_t i=0; i<col.count; ++i) {
            float* target = data_base + (i * col.vector_dim);
            float dist = l2_sq_simd(query.data(), target, col.vector_dim);
            results.push_back({i, dist});
        }
        std::partial_sort(results.begin(), results.begin() + std::min((size_t)top_k, results.size()), results.end(), 
            [](const SearchResult& a, const SearchResult& b) { return a.score < b.score; });
        if (results.size() > top_k) results.resize(top_k);
        return results;
    }

    // --- FILTER IMPLEMENTATION ---
    std::vector<RowID> Filter(const std::string& col_name, const std::string& op_raw, const std::string& val_str, int limit = -1) {
        if (column_map.find(col_name) == column_map.end()) throw std::runtime_error("Column not found: " + col_name);
        int col_idx = column_map[col_name];
        ColumnHeader& col = columns[col_idx];
        
        std::string op = str_to_upper(op_raw);
        std::vector<RowID> matches;
        matches.reserve(limit > 0 ? limit : 128); 

        if (col.type == ColType::INT32) {
            int32_t target = std::stoi(val_str);
            int32_t* data = get_ptr<int32_t>(col.data_offset);
            for (uint32_t i = 0; i < col.count; ++i) {
                if (limit > 0 && matches.size() >= (size_t)limit) break;
                bool match = false;
                if (op == "EQ" || op == "=") match = (data[i] == target);
                else if (op == "NEQ" || op == "!=") match = (data[i] != target);
                else if (op == "GTE" || op == ">=") match = (data[i] >= target);
                else if (op == "LTE" || op == "<=") match = (data[i] <= target);
                else if (op == "GT" || op == ">") match = (data[i] > target);
                else if (op == "LT" || op == "<") match = (data[i] < target);
                if (match) matches.push_back(i);
            }
        }
        else if (col.type == ColType::FLOAT32) {
            float target = std::stof(val_str);
            float* data = get_ptr<float>(col.data_offset);
            for (uint32_t i = 0; i < col.count; ++i) {
                if (limit > 0 && matches.size() >= (size_t)limit) break;
                bool match = false;
                if (op == "EQ" || op == "=") match = (std::abs(data[i] - target) < 1e-6); 
                else if (op == "NEQ" || op == "!=") match = (std::abs(data[i] - target) > 1e-6);
                else if (op == "GTE" || op == ">=") match = (data[i] >= target);
                else if (op == "LTE" || op == "<=") match = (data[i] <= target);
                else if (op == "GT" || op == ">") match = (data[i] > target);
                else if (op == "LT" || op == "<") match = (data[i] < target);
                if (match) matches.push_back(i);
            }
        }
        else if (col.type == ColType::TEXT) {
            RelPtr* offsets = get_ptr<RelPtr>(col.data_offset);
            for (uint32_t i = 0; i < col.count; ++i) {
                if (limit > 0 && matches.size() >= (size_t)limit) break;
                if (offsets[i] == DELETED_FLAG) continue;
                std::string val = std::string(get_ptr<char>(offsets[i]));
                bool match = false;
                
                if (op == "EQ" || op == "=") match = (val == val_str);
                else if (op == "NEQ" || op == "!=") match = (val != val_str);
                else if (op == "CONTAINS") match = str_contains(val, val_str);
                else if (op == "NOT_CONTAINS") match = !str_contains(val, val_str);
                else if (op == "ILIKE") match = str_contains(str_to_lower(val), str_to_lower(val_str));
                else if (op == "STARTS_WITH") match = str_starts_with(val, val_str);
                else if (op == "ENDS_WITH") match = str_ends_with(val, val_str);
                
                if (match) matches.push_back(i);
            }
        }
        return matches;
    }

    uint32_t Count(const std::string& col_name) {
        if (column_map.find(col_name) == column_map.end()) throw std::runtime_error("Column not found: " + col_name);
        return columns[column_map.at(col_name)].count;
    }

    std::string GetStats() {
        std::stringstream ss;
        ss << "   Arena Used: " << head << " / " << capacity_bytes << " bytes\n";
        ss << "   Columns: " << columns.size() << "\n";
        for(const auto& col : columns) {
            ss << "    - " << col.name << " (Rows: " << col.count << ", Cap: " << col.capacity << ")\n";
        }
        return ss.str();
    }
};

// --- ARCHITECTURE ---

struct ColumnGroup {
    std::unique_ptr<ColumnarTiger> engine;
    std::string role; 
    
    ColumnGroup(size_t size_mb, std::string r) : engine(std::make_unique<ColumnarTiger>(size_mb)), role(r) {}
    // For loading
    ColumnGroup(std::unique_ptr<ColumnarTiger> eng, std::string r) : engine(std::move(eng)), role(r) {}

    void Reset() {
        engine->Reset();
    }
};

struct BigTable {
    std::string name;
    std::unordered_map<std::string, std::unique_ptr<ColumnGroup>> groups;

    ColumnarTiger* CreateGroup(const std::string& group_name, size_t size_mb, const std::string& role) {
        if (groups.find(group_name) != groups.end()) return groups[group_name]->engine.get();
        
        groups[group_name] = std::make_unique<ColumnGroup>(size_mb, role);
        return groups[group_name]->engine.get();
    }

    ColumnarTiger* GetGroup(const std::string& group_name) {
        if (groups.find(group_name) == groups.end()) throw std::runtime_error("Group not found: " + group_name);
        return groups.at(group_name)->engine.get();
    }
    
    void Reset() {
        for (auto& pair : groups) pair.second->Reset();
    }

    std::string GetStats() {
        std::stringstream ss;
        ss << " Table: " << name << "\n";
        for (const auto& pair : groups) {
            ss << "  Group: " << pair.first << " (Role: " << pair.second->role << ")\n";
            ss << pair.second->engine->GetStats();
        }
        return ss.str();
    }
};

class NativeMetaDB {
    std::unordered_map<std::string, std::unique_ptr<BigTable>> tables;
public:
    BigTable* CreateTable(const std::string& name) {
        if (tables.find(name) != tables.end()) return tables[name].get();
        auto table = std::make_unique<BigTable>();
        table->name = name;
        tables[name] = std::move(table);
        return tables[name].get();
    }
    BigTable* GetTable(const std::string& name) {
        if (tables.find(name) == tables.end()) throw std::runtime_error("Table not found");
        return tables.at(name).get();
    }
    
    void Save(const std::string& dir_path) {
        for(const auto& t_pair : tables) {
            std::string t_name = t_pair.first;
            BigTable* t = t_pair.second.get();
            for(const auto& g_pair : t->groups) {
                std::string g_name = g_pair.first;
                std::string filename = dir_path + "_" + t_name + "_" + g_name + ".tg"; 
                g_pair.second->engine->Save(filename);
            }
        }
        std::ofstream manifest(dir_path + "_manifest.txt");
        for(const auto& t_pair : tables) {
            for(const auto& g_pair : t_pair.second->groups) {
                manifest << t_pair.first << " " << g_pair.first << " " << g_pair.second->role << "\n";
            }
        }
        manifest.close();
    }

    void Load(const std::string& dir_path) {
        std::ifstream manifest(dir_path + "_manifest.txt");
        if (!manifest) throw std::runtime_error("Manifest not found");
        tables.clear();
        std::string t_name, g_name, role;
        while (manifest >> t_name >> g_name >> role) {
            BigTable* t = CreateTable(t_name); 
            std::string filename = dir_path + "_" + t_name + "_" + g_name + ".tg";
            auto loaded_engine = ColumnarTiger::Load(filename);
            t->groups[g_name] = std::make_unique<ColumnGroup>(std::move(loaded_engine), role);
        }
    }

    void Reset() {
        for (auto& pair : tables) pair.second->Reset();
    }

    std::string GetStats() {
        std::stringstream ss;
        ss << "\n=== NativeMetaDB Statistics ===\n";
        ss << "Total Tables: " << tables.size() << "\n";
        for (const auto& pair : tables) ss << pair.second->GetStats();
        ss << "===============================\n";
        return ss.str();
    }
};

// --- TGQL (TensorGraph Query Language) ENGINE ---

struct ExecutionResult {
    std::string msg;
    std::vector<RowID> rows;
    bool has_rows = false;
    ColumnarTiger* context_engine = nullptr; 
};

class TGQL {
private:
    static std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(' ');
        if (std::string::npos == first) return str;
        size_t last = str.find_last_not_of(' ');
        return str.substr(first, (last - first + 1));
    }

    static std::vector<std::string> parse_arguments(const std::string& args_str) {
        std::vector<std::string> args;
        std::string current;
        bool in_quote = false;
        bool in_bracket = false;
        for (char c : args_str) {
            if (c == '"') in_quote = !in_quote;
            else if (c == '[' && !in_quote) { in_bracket = true; current += c; }
            else if (c == ']' && !in_quote) { in_bracket = false; current += c; }
            else if (c == ',' && !in_quote && !in_bracket) {
                args.push_back(trim(current));
                current.clear();
            } else current += c;
        }
        if (!current.empty()) args.push_back(trim(current));
        return args;
    }

    static ColType parse_type(std::string type_str) {
        std::transform(type_str.begin(), type_str.end(), type_str.begin(), ::toupper);
        if (type_str == "INT" || type_str == "INTEGER") return ColType::INT32;
        if (type_str == "FLOAT") return ColType::FLOAT32;
        if (type_str == "TEXT" || type_str == "STRING") return ColType::TEXT;
        if (type_str.find("VECTOR") == 0) return ColType::VECTOR_F32;
        throw std::runtime_error("Unknown type: " + type_str);
    }

    static std::vector<float> parse_vector_data(std::string val) {
        if (val.empty()) return {}; 
        if (val.front() == '[') val = val.substr(1);
        if (val.empty()) return {};
        if (val.back() == ']') val.pop_back();
        std::vector<float> vec;
        std::stringstream ss(val);
        std::string item;
        while (std::getline(ss, item, ',')) {
            item = trim(item);
            if(!item.empty()) vec.push_back(std::stof(item));
        }
        return vec;
    }

    // New: Internal run to return struct
    static ExecutionResult RunInternal(NativeMetaDB& db, std::string cmd, std::vector<std::string> args) {
        ExecutionResult res;
        
        if (cmd == "CREATE_TABLE") {
            if (args.empty()) throw std::runtime_error("CREATE_TABLE requires name");
            db.CreateTable(args[0]);
            res.msg = "Table '" + args[0] + "' created (or exists).";
        }
        else if (cmd == "CREATE_GROUP") {
            if (args.size() < 4) throw std::runtime_error("Req: table, group, size_mb, role");
            BigTable* t = db.GetTable(args[0]);
            t->CreateGroup(args[1], std::stoi(args[2]), args[3]);
            res.msg = "Group '" + args[1] + "' created (or exists).";
        }
        else if (cmd == "CREATE_COLUMN") {
            if (args.size() < 4) throw std::runtime_error("Req: table, group, name, type, [dim]");
            BigTable* t = db.GetTable(args[0]);
            ColumnarTiger* tg = t->GetGroup(args[1]);
            int dim = 0;
            ColType type = parse_type(args[3]);
            if (type == ColType::VECTOR_F32) {
                if (args.size() >= 5) dim = std::stoi(args[4]);
                else throw std::runtime_error("VECTOR requires dimension");
            }
            tg->CreateColumn(args[2], type, 10000, dim); // Default 10K
            res.msg = "Column '" + args[2] + "' created.";
        }
        else if (cmd == "CREATE_COLUMN_SZ") {
            if (args.size() < 5) throw std::runtime_error("Req: table, group, name, type, size, [dim]");
            BigTable* t = db.GetTable(args[0]);
            ColumnarTiger* tg = t->GetGroup(args[1]);
            int dim = 0;
            ColType type = parse_type(args[3]);
            uint32_t sz = std::stoi(args[4]);
            if (type == ColType::VECTOR_F32) {
                if (args.size() >= 6) dim = std::stoi(args[5]);
                else throw std::runtime_error("VECTOR requires dimension");
            }
            tg->CreateColumn(args[2], type, sz, dim);
            res.msg = "Column '" + args[2] + "' created (sz=" + std::to_string(sz) + ").";
        }
        else if (cmd == "ADD_ROW") {
            if (args.size() < 2) throw std::runtime_error("Req: table, group");
            BigTable* t = db.GetTable(args[0]);
            ColumnarTiger* tg = t->GetGroup(args[1]);
            RowID rid = tg->AddRow();
            // Optional: Populate values if provided
            // ... (Simple version doesn't yet parse varargs for population)
            res.msg = "Row added. ID: " + std::to_string(rid);
        }
        else if (cmd == "UPDATE" || cmd == "SET") {
            if (args.size() < 5) throw std::runtime_error("Req: table, group, col, row_id, val");
            BigTable* t = db.GetTable(args[0]);
            ColumnarTiger* tg = t->GetGroup(args[1]);
            std::string col = args[2];
            RowID rid = std::stoi(args[3]);
            std::string val = args[4];

            if (val.find('[') != std::string::npos) tg->SetVector(col, rid, parse_vector_data(val));
            else if (val.find_first_not_of("0123456789-") == std::string::npos) tg->SetInt(col, rid, std::stoi(val));
            else if (val.find('.') != std::string::npos) {
                try { tg->SetFloat(col, rid, std::stof(val)); } 
                catch(...) { tg->SetText(col, rid, val); }
            } else tg->SetText(col, rid, val);
            res.msg = "Updated.";
        }
        else if (cmd == "GET") {
            if (args.size() < 4) throw std::runtime_error("Req: table, group, col, row_id");
            BigTable* t = db.GetTable(args[0]);
            ColumnarTiger* tg = t->GetGroup(args[1]);
            // Infer type or assume text for simple output?
            // Engine doesn't store type metadata easily accessible publically without 'GetColumnType'
            // Added helper above.
            ColType type = tg->GetColumnType(args[2]);
            if (type == ColType::TEXT) res.msg = tg->GetText(args[2], std::stoi(args[3]));
            else if (type == ColType::INT32) res.msg = std::to_string(tg->GetInt(args[2], std::stoi(args[3])));
            else if (type == ColType::FLOAT32) res.msg = std::to_string(tg->GetFloat(args[2], std::stoi(args[3])));
            else res.msg = "[VECTOR]";
        }
        else if (cmd == "SELECT") {
            if (args.size() < 5) throw std::runtime_error("Req: table, group, col, limit, offset");
            BigTable* t = db.GetTable(args[0]);
            ColumnarTiger* tg = t->GetGroup(args[1]);
            std::string col = args[2];
            int limit = std::stoi(args[3]);
            int offset = std::stoi(args[4]);

            if (limit<=0) {
                throw std::runtime_error("LIMIT MUST BE POSITIVE");
            }
            
            ColType type = tg->GetColumnType(col);
            uint32_t total = tg->GetRowCount(col);
            if ((uint32_t)offset >= total) { res.msg = "Offset out of bounds"; return res; }
            int end = std::min((int)total, offset + limit);
            
            std::stringstream ss;
            for (int i = offset; i < end; ++i) {
                if (type == ColType::TEXT) ss << tg->GetText(col, i) << "\n";
                else if (type == ColType::INT32) ss << tg->GetInt(col, i) << "\n";
                else if (type == ColType::FLOAT32) ss << tg->GetFloat(col, i) << "\n";
            }
            res.msg = ss.str();
        }
        else if (cmd == "FILTER") {
            // FILTER(table, group, col, op, val, [limit])
            if (args.size() < 5) throw std::runtime_error("Req: table, group, col, op, val");
            BigTable* t = db.GetTable(args[0]);
            ColumnarTiger* tg = t->GetGroup(args[1]);
            int limit = (args.size() > 5) ? std::stoi(args[5]) : -1;

            if (limit<=0) {
                throw std::runtime_error("LIMIT MUST BE POSITIVE");
            }
            
            res.rows = tg->Filter(args[2], args[3], args[4], limit);
            res.has_rows = true;
            res.context_engine = tg;
            res.msg = "Found " + std::to_string(res.rows.size()) + " matches.";
        }
        else if (cmd == "SEARCH") {
            if (args.size() < 5) throw std::runtime_error("Req: table, group, col, vec, top_k");
            BigTable* t = db.GetTable(args[0]);
            ColumnarTiger* tg = t->GetGroup(args[1]);
            auto results = tg->VectorSearch(args[2], parse_vector_data(args[3]), std::stoi(args[4]));
            
            res.context_engine = tg;
            res.has_rows = true;
            for(auto& r : results) res.rows.push_back(r.id);
            res.msg = "Found " + std::to_string(res.rows.size()) + " vectors.";
        }
        else if (cmd == "COUNT") {
            if (args.size() < 3) throw std::runtime_error("Req: table, group, col");
            BigTable* t = db.GetTable(args[0]);
            ColumnarTiger* tg = t->GetGroup(args[1]);
            res.msg = std::to_string(tg->Count(args[2]));
        }
        else if (cmd == "COUNT_IF" || cmd == "COUNT_FILTER") {
            if (args.size() < 5) throw std::runtime_error("Req: table, group, col, op, val");
            BigTable* t = db.GetTable(args[0]);
            ColumnarTiger* tg = t->GetGroup(args[1]);
            auto rows = tg->Filter(args[2], args[3], args[4]);
            res.msg = std::to_string(rows.size());
        }
        else if (cmd == "SAVE") {
            if (args.size() < 1) throw std::runtime_error("Req: path");
            db.Save(args[0]);
            res.msg = "Saved.";
        }
        else if (cmd == "LOAD") {
            if (args.size() < 1) throw std::runtime_error("Req: path");
            db.Load(args[0]);
            res.msg = "Loaded.";
        }
        else if (cmd == "STATS") {
             res.msg = db.GetStats();
        }
        else if (cmd == "RESET") {
             db.Reset();
             res.msg = "Reset.";
        }
        else {
            throw std::runtime_error("Unknown command: " + cmd);
        }
        return res;
    }

public:
    static std::string Execute(NativeMetaDB& db, std::string query) {
        try {
            query = trim(query);
            if (query.empty()) return "";
            if (query.back() == ';') query.pop_back();

            // Handle Pipeline |>
            size_t pipe_pos = query.find("|>");
            std::string lhs = (pipe_pos == std::string::npos) ? query : query.substr(0, pipe_pos);
            std::string rhs = (pipe_pos == std::string::npos) ? "" : query.substr(pipe_pos + 2);

            // Execute LHS
            size_t paren_pos = lhs.find('(');
            if (paren_pos == std::string::npos) {
                // Handle no-arg cmds like STATS/EXIT if passed as string directly (rare in this flow)
                if (trim(lhs) == "STATS") return db.GetStats();
                return "Error: Invalid syntax";
            }
            std::string cmd = trim(lhs.substr(0, paren_pos));
            std::string args_content = lhs.substr(paren_pos + 1);
            if (args_content.find(')') != std::string::npos) {
                args_content = args_content.substr(0, args_content.find_last_of(')'));
            }
            
            std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
            std::vector<std::string> args = parse_arguments(args_content);
            
            ExecutionResult res = RunInternal(db, cmd, args);

            // Handle RHS (Project)
            if (!rhs.empty() && res.has_rows && res.context_engine) {
                rhs = trim(rhs);
                if (rhs.find("project") == 0) {
                     size_t p_start = rhs.find('(');
                     size_t p_end = rhs.find(')');
                     if (p_start != std::string::npos && p_end != std::string::npos) {
                         std::string cols_str = rhs.substr(p_start+1, p_end - p_start - 1);
                         std::vector<std::string> cols = parse_arguments(cols_str);
                         
                         std::stringstream ss;
                         for (RowID r : res.rows) {
                             for (size_t i=0; i<cols.size(); ++i) {
                                 std::string cname = cols[i];
                                 ColType type = res.context_engine->GetColumnType(cname);
                                 if (type == ColType::TEXT) ss << res.context_engine->GetText(cname, r);
                                 else if (type == ColType::INT32) ss << res.context_engine->GetInt(cname, r);
                                 else if (type == ColType::FLOAT32) ss << res.context_engine->GetFloat(cname, r);
                                 if (i < cols.size()-1) ss << ", ";
                             }
                             ss << "\n";
                         }
                         return ss.str();
                     }
                }
                return "Error: Unknown pipeline command";
            }

            // Default output for non-piped commands
            if (res.has_rows) {
                 // If just returning IDs
                 std::stringstream ss;
                 ss << "[";
                 for(size_t i=0; i<res.rows.size(); ++i) {
                     ss << res.rows[i] << (i < res.rows.size()-1 ? "," : "");
                 }
                 ss << "]";
                 return ss.str();
            }

            return res.msg;

        } catch (const std::exception& e) {
            return std::string("Error: ") + e.what();
        }
    }
};