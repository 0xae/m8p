/*
TENSORGRAPH DB
Ayrton Gomes 2025-12-16

I think this storage engine is very interesting,it would allow me to build a storage architecture comprise of Tables and ColumnGroups
this allows a table to have multiple TensorGraph db each one representing a set of columns for a particular requirement.
So for example i know a Company has many officers, Cases, has its detail, this architecture allows me to quickly detect all these
children and scatter-gatter a parralel query across its columngroup and also allows for cross relationships to form and to be queries fast
because there's no burden of IPC and all that a sqlengine entails.

What do you think.

=======================================================================
                            BIGTABLE COMPANY
=======================================================================
Metatable: Details  | id:int32  | name:TEXT | status:TEXT | sales:FLOAT
-----------------------------------------------------------------------
Metatable: Officers | name:TEXT | role:TEXT
-----------------------------------------------------------------------
Metatable: Cases    | date:DATE | case_descr:VECTOR[453]
-----------------------------------------------------------------------
=======================================================================

CREATE_TABLE(company);

CREATE_GROUP(company, Details, 64, "metadata");
CREATE_COLUMN(company, Details, name, TEXT);
CREATE_COLUMN(company, Details, number, TEXT);
CREATE_COLUMN(company, Details, status, TEXT);

CREATE_GROUP(company, Officers, 64, "relations");
CREATE_COLUMN(company, Officers, name, TEXT);
CREATE_COLUMN(company, Officers, role, TEXT);

CREATE_GROUP(company, Cases, 64, "cases-at-court");
CREATE_COLUMN(company, Cases, date, DATE);
CREATE_COLUMN(company, Cases, case_descr, VECTOR[453]);

SO LIKE THIS

struct TableDetails {
 std::string tablename;
 // ...
 std::unordered_map<std::string, *TensorGraph> ColumnGroup;
}

std::unordered_map<std::string, TableDetails> DB;

* ## Define Columns const int MAX_PER_COLUMN = 10000;

tg_exec <r3> CREATE_TABLE(company);

tg_exec <r3> CREATE_GROUP(company, Details, 64, "metadata");
tg_exec <r3> CREATE_COLUMN(company, Details, name, TEXT);
tg_exec <r3> CREATE_COLUMN(company, Details, number, TEXT);
tg_exec <r3> CREATE_COLUMN(company, Details, status, TEXT);

tg_exec <r3> CREATE_GROUP(company, Officers, 64, "relations");
tg_exec <r3> CREATE_COLUMN(company, Officers, name, TEXT);
tg_exec <r3> CREATE_COLUMN(company, Officers, role, TEXT);

tg_exec <r3> CREATE_GROUP(company, Embed, 128, "vectors");
tg_exec <r3> CREATE_COLUMN(company, Embed, face_vec, VECTOR, 128); 


## INDEX
tg_exec <r3> ADD_ROW(company, Details); 
tg_exec <r3> UPDATE(company, Details, name, 0, My Favorite Company);
tg_exec <r3> UPDATE(company, Details, number, 0, "AC00123");
tg_exec <r3> UPDATE(company, Details, status, 0, In Administration);
assertcontains <r3> 

tg_exec <r3> ADD_ROW(company, Details); 
tg_exec <r3> UPDATE(company, Details, name, 1, Another Company);
tg_exec <r3> UPDATE(company, Details, number, 1, "A9923");
tg_exec <r3> UPDATE(company, Details, status, 1, In Liquidation);

tg_exec <r3> ADD_ROW(company, Details); 
tg_exec <r3> UPDATE(company, Details, name, 2, Third Company);
tg_exec <r3> UPDATE(company, Details, number, 2, "A2323");
tg_exec <r3> UPDATE(company, Details, status, 2, Active);

## QUERY

tg_exec <rnames> SELECT(company, Details, name, 10, 0, TEXT);
tg_exec <rnumbers> SELECT(company, Details, number, 10, 0, TEXT);
tg_exec <rstatus> SELECT(company, Details, status, 10, 0, TEXT);

ret <rnames> <rnumbers> <rstatus>
*/

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
const int MAX_ROWS_P_COLUMN = 100000000; // 100M

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
// Force target attributes to ensure FMA instructions are available if AVX2/AVX512 is enabled
#if defined(__GNUC__) || defined(__clang__)
    #if defined(__AVX512F__)
    __attribute__((target("avx512f")))
    #elif defined(__AVX2__)
    __attribute__((target("avx2,fma")))
    #endif
#endif
inline float l2_sq_simd(const float* a, const float* b, int dim) {
#if defined(__AVX512F__)
    // AVX-512 Implementation (16 floats at a time)
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
    
    // Horizontal sum of the 8 float lanes
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

// --- TensorGraph Engine ---
class TensorGraph {
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
    friend class TGQL;

public:
    TensorGraph(size_t size_mb) {
        capacity_bytes = size_mb * 1024 * 1024;
        memory_block = new uint8_t[capacity_bytes]; 
        std::memset(memory_block, 0, capacity_bytes);
        head = 0; 
    }

    ~TensorGraph() { delete[] memory_block; }

    // --- Efficient Soft Reset ---
    // Resets allocator head and clears metadata.
    // Does NOT deallocate the memory_block (avoids syscall overhead).
    // O(1) complexity.
    void Reset() {
        head = 0;
        columns.clear();
        column_map.clear();
        // No memset required; old data will be overwritten naturally.
    }

    void Save(const std::string& filepath) {
        std::ofstream out(filepath, std::ios::binary);
        if (!out) throw std::runtime_error("Cannot open file for writing: " + filepath);

        // 1. Write Capacity and Head
        out.write(reinterpret_cast<const char*>(&capacity_bytes), sizeof(size_t));
        out.write(reinterpret_cast<const char*>(&head), sizeof(size_t));

        // 2. Write Columns Metadata
        size_t num_cols = columns.size();
        out.write(reinterpret_cast<const char*>(&num_cols), sizeof(size_t));
        if (num_cols > 0) {
            out.write(reinterpret_cast<const char*>(columns.data()), num_cols * sizeof(ColumnHeader));
        }

        // 3. Write Memory Block (Only used portion)
        out.write(reinterpret_cast<const char*>(memory_block), head);
        out.close();
    }

    static std::unique_ptr<TensorGraph> Load(const std::string& filepath) {
        std::ifstream in(filepath, std::ios::binary);
        if (!in) throw std::runtime_error("Cannot open file for reading: " + filepath);

        size_t cap, hd;
        in.read(reinterpret_cast<char*>(&cap), sizeof(size_t));
        in.read(reinterpret_cast<char*>(&hd), sizeof(size_t));

        auto db = std::make_unique<TensorGraph>(cap / (1024 * 1024)); // Reconstruct with MB size
        db->head = hd;

        size_t num_cols;
        in.read(reinterpret_cast<char*>(&num_cols), sizeof(size_t));
        db->columns.resize(num_cols);
        if (num_cols > 0) {
            in.read(reinterpret_cast<char*>(db->columns.data()), num_cols * sizeof(ColumnHeader));
        }

        // Rebuild map
        for (int i = 0; i < num_cols; ++i) {
            db->column_map[db->columns[i].name] = i;
        }

        in.read(reinterpret_cast<char*>(db->memory_block), hd);
        in.close();
        return db;
    }

    void CreateColumn(const std::string& name, ColType type, uint32_t initial_capacity, int dim = 0) {
        if (name.size()>30) {
            throw std::runtime_error("COLUMN SIZE CANNOT 30 CHARS");   
        }

        if (column_map.find(name) != column_map.end()) {
            // throw std::runtime_error("Column exists");
            // column already created
            return;
        }

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

    std::vector<RowID> Filter(const std::string& col_name, const std::string& op_raw, const std::string& val_str) {
        if (column_map.find(col_name) == column_map.end()) throw std::runtime_error("Column not found: " + col_name);
        int col_idx = column_map[col_name];
        ColumnHeader& col = columns[col_idx];
        
        std::string op = str_to_upper(op_raw);
        std::vector<RowID> matches;
        matches.reserve(col.count > 128 ? 128 : col.count); 

        if (col.type == ColType::INT32) {
            int32_t target = std::stoi(val_str);
            int32_t* data = get_ptr<int32_t>(col.data_offset);
            for (uint32_t i = 0; i < col.count; ++i) {
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
                bool match = false;
                // Using 1e-6 epsilon for float equality
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
                if (offsets[i] == DELETED_FLAG) continue;
                std::string val = std::string(get_ptr<char>(offsets[i]));
                bool match = false;
                
                if (op == "EQ" || op == "=") match = (val == val_str);
                else if (op == "NEQ" || op == "!=") match = (val != val_str);
                else if (op == "CONTAINS") match = str_contains(val, val_str);
                else if (op == "ILIKE") match = str_contains(str_to_lower(val), str_to_lower(val_str));
                else if (op == "STARTS_WITH") match = str_starts_with(val, val_str);
                else if (op == "ENDS_WITH") match = str_ends_with(val, val_str);
                
                if (match) matches.push_back(i);
            }
        }
        else {
            throw std::runtime_error("Filtering not supported for this column type");
        }
        return matches;
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
    std::unique_ptr<TensorGraph> engine;
    std::string role; 
    
    ColumnGroup(size_t size_mb, std::string r) : engine(std::make_unique<TensorGraph>(size_mb)), role(r) {}
    // For loading
    ColumnGroup(std::unique_ptr<TensorGraph> eng, std::string r) : engine(std::move(eng)), role(r) {}

    void Reset() {
        engine->Reset();
    }
};

struct BigTable {
    std::string name;
    std::unordered_map<std::string, std::unique_ptr<ColumnGroup>> groups;

    TensorGraph* CreateGroup(const std::string& group_name, size_t size_mb, const std::string& role) {
        // IDEMPOTENT: If group exists, return it (it might be reset)
        if (groups.find(group_name) != groups.end()) return groups[group_name]->engine.get();
        
        groups[group_name] = std::make_unique<ColumnGroup>(size_mb, role);
        return groups[group_name]->engine.get();
    }

    TensorGraph* GetGroup(const std::string& group_name) {
        if (groups.find(group_name) == groups.end()) throw std::runtime_error("Group not found: " + group_name);
        return groups.at(group_name)->engine.get();
    }
    
    void Reset() {
        for (auto& pair : groups) {
            pair.second->Reset();
        }
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
    friend class TGQL;
public:
    BigTable* CreateTable(const std::string& name) {
        // IDEMPOTENT: If table exists, return it (so scripts can re-run after reset)
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
    
    // --- Persistence ---
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
            // Because CreateTable is idempotent, this works nicely
            BigTable* t = CreateTable(t_name); 
            
            std::string filename = dir_path + "_" + t_name + "_" + g_name + ".tg";
            auto loaded_engine = TensorGraph::Load(filename);
            t->groups[g_name] = std::make_unique<ColumnGroup>(std::move(loaded_engine), role);
        }
    }

    void Reset() {
        for (auto& pair : tables) {
            pair.second->Reset();
        }
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


class TGQL {
private:
    static std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(' ');
        if (std::string::npos == first) return str;
        size_t last = str.find_last_not_of(' ');
        return str.substr(first, (last - first + 1));
    }

    static std::vector<std::string> split(std::string &s, std::string delimiter) {
        size_t pos_start = 0, pos_end, delim_len = delimiter.length();
        std::string token;
        std::vector<std::string> res;

        while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
            token = s.substr (pos_start, pos_end - pos_start);
            pos_start = pos_end + delim_len;
            trim(token);
            if (token.size()>0) {                
                res.push_back (token);
            }
        }

        res.push_back(s.substr(pos_start));
        return res;
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

public:
    // Execute a query string against the DB and return the result (or error string)
    static std::string Execute(NativeMetaDB& db, std::string query) {
        try {
            query = trim(query);
            if (query.find("--")!=std::string::npos) {
                query = query.substr(0, query.find("--"));
            }
            if (query.find("#")!=std::string::npos) {
                query = query.substr(0, query.find("#"));
            }

            if (query.empty()) return "";
            if (query.back() == ';') query.pop_back();

            size_t paren_pos = query.find('(');
            if (paren_pos == std::string::npos) {
                std::string cmd = query;
                std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
                if (cmd == "STATS") return db.GetStats();
                if (cmd == "RESET") { db.Reset(); return "Database reset successfully."; }
                return "Error: Invalid syntax. Expected COMMAND(args)";
            }

            std::string cmd = trim(query.substr(0, paren_pos));
            std::string args_content = query.substr(paren_pos + 1);
            if (args_content.back() == ')') args_content.pop_back();

            // std::cout << "args_content: " << args_content
            //     << "\n" << std::endl;

            std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
            std::vector<std::string> args = parse_arguments(args_content);
            std::stringstream result_ss;

            if (cmd == "CREATE_TABLE") {
                if (args.empty()) throw std::runtime_error("CREATE_TABLE requires name");
                db.CreateTable(args[0]);
                return "Table '" + args[0] + "' created (or exists).";
            }
            else if (cmd == "CREATE_GROUP") {
                if (args.size() < 4) throw std::runtime_error("Req: table, group, size_mb, role");
                BigTable* t = db.GetTable(args[0]);
                t->CreateGroup(args[1], std::stoi(args[2]), args[3]);
                return "Group '" + args[1] + "' created (or exists).";
            }
            else if (cmd == "CREATE_COLUMN") {
                if (args.size() < 4) {
                    throw std::runtime_error("Req: table, group, name, type, [dim]");
                }

                BigTable* t = db.GetTable(args[0]);
                TensorGraph* tg = t->GetGroup(args[1]);
                int dim = 0;
                ColType type = parse_type(args[3]);
                if (type == ColType::VECTOR_F32) {
                    if (args.size() >= 5) {
                        dim = std::stoi(args[4]);
                    } else {
                        throw std::runtime_error("VECTOR requires dimension");
                    }
                }

                tg->CreateColumn(args[2], type, 10000, dim);
                return "Column '" + args[2] + "' created.";
            }
            else if (cmd == "CREATE_COLUMN_SZ") {
                if (args.size() < 5) {
                    throw std::runtime_error("Req: table, group, name, type, max_elements, [dim]");
                }

                BigTable* t = db.GetTable(args[0]);
                TensorGraph* tg = t->GetGroup(args[1]);
                int dim = 0;
                ColType type = parse_type(args[3]);
                int max_elements = std::stoi(args[4]);
                if (type == ColType::VECTOR_F32) {
                    if (args.size() >= 6) {
                        dim = std::stoi(args[5]);
                    } else {
                        throw std::runtime_error("VECTOR requires dimension");
                    }
                }

                if (max_elements >= MAX_ROWS_P_COLUMN) {
                    throw std::runtime_error("max_elements cannot be larger than "+std::to_string(MAX_ROWS_P_COLUMN));
                }

                tg->CreateColumn(args[2], type, max_elements, dim);
                return "Column '" + args[2] + "' created with max_elements=." + std::to_string(max_elements);
            }
            else if (cmd == "ADD_ROW") {
                if (args.size() < 2) throw std::runtime_error("Req: table, group");
                BigTable* t = db.GetTable(args[0]);
                TensorGraph* tg = t->GetGroup(args[1]);
                RowID rid = tg->AddRow();
                return "Row added. ID: " + std::to_string(rid);
            }
            else if (cmd == "UPDATE" || cmd == "SET") {
                if (args.size() < 5) throw std::runtime_error("Req: table, group, col, row_id, val");
                BigTable* t = db.GetTable(args[0]);
                TensorGraph* tg = t->GetGroup(args[1]);
                std::string col = args[2];
                RowID rid = std::stoi(args[3]);
                std::string val = args[4];

                if (!(tg->column_map.count(col))) {
                    throw std::runtime_error("COLUMN["+col+"] NOT FOUND");                    
                }

                auto &colRef = tg->columns[tg->column_map[col]];
                std::string typeL="";
                if (colRef.type == ColType::INT32) {
                    typeL = "INT";
                } else if (colRef.type == ColType::FLOAT32)  {
                    typeL = "FLOAT";
                } else if (colRef.type == ColType::TEXT) {
                    typeL = "TEXT";
                } else if (colRef.type == ColType::VECTOR_F32) {
                    typeL = "VECTOR";
                }

                if (typeL=="VECTOR") {
                    tg->SetVector(col, rid, parse_vector_data(val));

                } else if (typeL=="INT") {
                    tg->SetInt(col, rid, std::stoi(val));

                } else if (typeL=="FLOAT") {
                    tg->SetFloat(col, rid, std::stoi(val));

                } else {
                    tg->SetText(col, rid, val);
                }

                return "Updated.";
            }
            else if (cmd == "GET") {
                if (args.size()<4) {
                    throw std::runtime_error("Req: table, group, col, row_id, TYPE");
                }

                BigTable* t = db.GetTable(args[0]);
                TensorGraph* tg = t->GetGroup(args[1]);
                std::string colName = args[2];
                if (!(tg->column_map.count(colName))) {
                    throw std::runtime_error("COLUMN["+colName+"] NOT FOUND");                    
                }

                // std::transform(type.begin(), type.end(), type.begin(), ::toupper);
                auto &col = tg->columns[tg->column_map[colName]];
                std::string typeL="";
                if (col.type == ColType::INT32) {
                    typeL = "INT";
                } else if (col.type == ColType::FLOAT32)  {
                    typeL = "FLOAT";
                } else if (col.type == ColType::TEXT) {
                    typeL = "TEXT";
                } else if (col.type == ColType::VECTOR_F32) {
                    typeL = "VECTOR";
                }

                if (typeL == "TEXT") return tg->GetText(colName, std::stoi(args[3]));
                else if (typeL == "INT") return std::to_string(tg->GetInt(colName, std::stoi(args[3])));
                else if (typeL == "FLOAT") return std::to_string(tg->GetFloat(colName, std::stoi(args[3])));
                return "Unknown type hint";
            }
            else if (cmd == "SELECT") {
                if (args.size() < 5) throw std::runtime_error("Req: table, group, col, limit, offset");
                BigTable* t = db.GetTable(args[0]);
                TensorGraph* tg = t->GetGroup(args[1]);
                std::string col = args[2];
                int limit = std::stoi(args[3]);
                int offset = std::stoi(args[4]);
                // std::string type = args[5];

                if (!(tg->column_map.count(col))) {
                    throw std::runtime_error("COLUMN["+col+"] NOT FOUND");
                }

                auto &colRef = tg->columns[tg->column_map[col]];
                std::string type="";
                if (colRef.type == ColType::INT32) {
                    type = "INT";
                } else if (colRef.type == ColType::FLOAT32)  {
                    type = "FLOAT";
                } else if (colRef.type == ColType::TEXT) {
                    type = "TEXT";
                } else if (colRef.type == ColType::VECTOR_F32) {
                    type = "VECTOR";
                }

                uint32_t total = tg->GetRowCount(col);
                if ((uint32_t)offset >= total) {
                    return "Offset out of bounds";
                }
                int end = std::min((int)total, offset + limit);
                
                result_ss << "Rows " << offset << "-" << (end-1) << ":\n";
                for (int i = offset; i < end; ++i) {
                    result_ss << "[" << i << "] ";
                    if (type == "TEXT") result_ss << tg->GetText(col, i) << "\n";
                    else if (type == "INT") result_ss << tg->GetInt(col, i) << "\n";
                    else if (type == "FLOAT") result_ss << tg->GetFloat(col, i) << "\n";
                    else if (type == "VECTOR") {
                        auto vec = tg->GetVector(col, i);
                        result_ss << "[";
                        for(size_t v=0; v<vec.size(); ++v) result_ss << vec[v] << (v < vec.size()-1 ? ", " : "");
                        result_ss << "]\n";
                    }
                }
                return result_ss.str();
            }
            else if (cmd == "FILTER") {
                if (args.size() < 5) {
                    throw std::runtime_error("FILTER (table, group, col, operator, value, [limit]) |> project(field,field2,...)");
                }
                auto &colName = args[2];
                BigTable* t = db.GetTable(args[0]);
                TensorGraph* tg = t->GetGroup(args[1]);
                auto results = tg->Filter(colName, args[3], args[4]);

                if (results.size()==0){
                    return "[]";
                }
                
                // result_ss << "Found " << results.size() << "-matches: ";
                int limit = 20; 
                if (args.size()>5) {
                    limit = std::stoi(args[5]);
                }

                int count = 0;
                std::vector<std::string> rows;
                const std::string filter_syntax = "|> project(";
                auto pjt_idx=args_content.find(filter_syntax);
                auto should_project = (pjt_idx!=std::string::npos);
                std::vector<std::string> pcols;

                if (should_project) {
                    std::string project_cols = args_content.substr(pjt_idx+filter_syntax.size());
                    pcols = split(project_cols, ",");
                } else {
                    result_ss << "[";
                }

                for(auto id : results) {
                    if (count++ >= limit) {
                        break;
                    }

                    if (pcols.size()>0) {
                        result_ss  << "rowid=" << id << " | ";
                        for (auto COLUMN_NAME : pcols) {
                            if (tg->column_map.count(COLUMN_NAME)) {
                                auto &col = tg->columns[tg->column_map[COLUMN_NAME]];
                                std::string typeL="";
                                if (col.type == ColType::INT32) {
                                    typeL = "INT";
                                } else if (col.type == ColType::FLOAT32)  {
                                    typeL = "FLOAT";
                                } else if (col.type == ColType::TEXT) {
                                    typeL = "TEXT";
                                } else if (col.type == ColType::VECTOR_F32) {
                                    typeL = "VECTOR";
                                } else {
                                    throw std::runtime_error("COLUMN TYPE NOT FOUND: " + COLUMN_NAME);
                                }

                                // std::string COMMAND =  "GET("+args[0]+", "+args[1]+", "+args[2]+", "+std::to_string(id)+", "+typeL+")";
                                std::string COMMAND =  "GET("+args[0]+", "+args[1]+", "+COLUMN_NAME+", "+std::to_string(id)+", "+typeL+")";
                                // std::cout << "COMMAND: " << COMMAND << "\n" << std::endl;
                                std::string result__x = Execute(db, COMMAND);
                                result_ss  << result__x << " | ";                     
                            } else {
                                throw std::runtime_error("COLUMN NOT FOUND: " + COLUMN_NAME);
                            }
                        }
                        result_ss  << "\n";
                    } else {
                        result_ss << id << ","; 
                    }
                }

                if (pcols.size()==0) {
                    result_ss << "]";
                }

                if (count>limit) {
                    result_ss << "More=" << (results.size() - limit);
                }
                result_ss << "| Found=" << std::to_string(results.size());

                // if (pjt_idx != std::string::npos) {
                //     std::string project_cols = args_content.substr(pjt_idx);
                // }

                return result_ss.str();
            }
            else if (cmd == "SEARCH") {
                if (args.size() < 5) throw std::runtime_error("Req: table, group, col, [vec], top_k");
                BigTable* t = db.GetTable(args[0]);
                TensorGraph* tg = t->GetGroup(args[1]);
                auto results = tg->VectorSearch(args[2], parse_vector_data(args[3]), std::stoi(args[4]));
                result_ss << "Results:\n";
                for(auto& r : results) result_ss << " - ID: " << r.id << " Score: " << r.score << "\n";
                return result_ss.str();
            }
            // Add Persistence Commands
            else if (cmd == "SAVE") {
                if (args.size() < 1) throw std::runtime_error("SAVE requires directory path prefix");
                db.Save(args[0]);
                return "Database saved to prefix: " + args[0];
            }
            else if (cmd == "LOAD") {
                if (args.size() < 1) throw std::runtime_error("LOAD requires directory path prefix");
                db.Load(args[0]);
                return "Database loaded from prefix: " + args[0];
            }
            
            return "Unknown command: " + cmd;

        } catch (const std::exception& e) {
            return std::string("Error: ") + e.what();
        }
    }
};