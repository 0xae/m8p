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
#include <unordered_set>
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

const size_t MAX_SK_CHUNK_SIZE = 1000;

// --- Types ---
using RelPtr = uint32_t;
using RowID = uint32_t;

class TGQL;

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

std::string normalize_token(const std::string& input, size_t limit = 200) {
    std::string result;
    result.reserve(std::min(input.size(), limit));

    for (char c : input) {
        if (result.size() >= limit) break;
        
        // Skip spaces, newlines, dots, commas
        if (std::isspace(static_cast<unsigned char>(c)) || 
            c == ',' || c == '\n' || c == '\r') {
            continue;
        }
        
        // Normalize to lowercase
        result.push_back(std::tolower(static_cast<unsigned char>(c)));
    }
    return result;
}

std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    
    if (s.empty()) {
        return tokens;
    }

    // Optimization: Reserve space to minimize reallocations.
    // Assuming average token length is small, s.length() / 4 is a heuristic.
    tokens.reserve(s.length() / 4);
    
    size_t start = 0;
    size_t end = s.find(delimiter);
    
    while (end != std::string::npos) {
        tokens.push_back(s.substr(start, end - start));
        start = end + 1;
        end = s.find(delimiter, start);
    }
    
    // Push the remaining part of the string (or empty string if ends with delimiter)
    tokens.push_back(s.substr(start));
    
    return tokens;
}


struct IndexData {
    std::unordered_map<std::string, std::vector<RowID>> map;
    uint32_t last_indexed_row = 0; // High-water mark for incremental updates
};

// --- ColumnarTiger Engine ---
class ColumnarTiger {
private:
    friend class TGQL;
    uint8_t* memory_block;
    size_t capacity_bytes;
    size_t head; 
    uint32_t version_watermark = 0;
    
    std::unordered_map<std::string, int> column_map; 
    std::vector<ColumnHeader> columns;

    // --- Indexes ---
    // Map: Column Name -> (Value -> List of RowIDs)
    // Currently optimized for TEXT columns (exact match)
    std::unordered_map<std::string, std::unordered_map<std::string, std::vector<RowID>>> indexes;
    std::unordered_map<std::string, IndexData> sk_indexes;

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
    std::vector<std::string> parse_in_list(std::string val) {
        if (val.empty()) return {};
        // Strip brackets if present
        if (val.front() == '[') val = val.substr(1);
        if (!val.empty() && val.back() == ']') val.pop_back();
        
        std::vector<std::string> res;
        std::stringstream ss(val);
        std::string item;
        while (std::getline(ss, item, ',')) {
            // trim whitespace
            size_t first = item.find_first_not_of(" ");
            if (first == std::string::npos) continue;
            size_t last = item.find_last_not_of(" ");
            std::string clean = item.substr(first, last - first + 1);
            
            // Remove quotes if present
            if (clean.size() >= 2 && clean.front() == '"' && clean.back() == '"') {
                clean = clean.substr(1, clean.size() - 2);
            }
            res.push_back(clean);
        }
        return res;
    }

public:
    ColumnarTiger(size_t size_mb) {
        capacity_bytes = size_mb * 1024 * 1024;
        memory_block = new uint8_t[capacity_bytes]; 
        std::memset(memory_block, 0, capacity_bytes);
        head = 0; 
        version_watermark = 0;
    }

    ~ColumnarTiger() { delete[] memory_block; }

    void Reset() {
        head = 0;
        version_watermark = 0;
        columns.clear();
        column_map.clear();
        indexes.clear();
    }

    // Accessors for watermark (used by indexer)
    uint32_t GetWatermark() const { return version_watermark; }
    void SetWatermark(uint32_t wm) { version_watermark = wm; }

    void ClearIndex(const std::string &col_name) {
        if (column_map.find(col_name) == column_map.end()) {
            throw std::runtime_error("Column not found: " + col_name);
        }

        if (HasIndex(col_name)) {
            return;            
        }

        if (indexes.count(col_name)) {
            indexes[col_name].clear();
        }
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
        // Note: Indexes are rebuilt on load, not saved (for simplicity).
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
        if (column_map.find(name) != column_map.end()) {
            // throw std::runtime_error("Column exists");
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

        // Check if column exists and get type
    bool HasColumn(const std::string& name) { return column_map.find(name) != column_map.end(); }
    ColType GetColumnType(const std::string& name) { return columns[column_map.at(name)].type; }

    // --- Index Management ---
    // void UpdateSecondaryIndex(const std::string& col_name) {
    //     const std::string index_name = col_name + "_sk";
    //     if (column_map.find(col_name) == column_map.end()) {
    //         throw std::runtime_error("Column not found: " + col_name);
    //     }

    //     if (!HasIndex(index_name)) {
    //         return;
    //     }

    //     // std::unordered_map<std::string, std::vector<RowID>> &idx = indexes[index_name];
    //     // if (idx.size()>=col.count) {
    //     //     return;
    //     // }
    //     // RelPtr* offsets = get_ptr<RelPtr>(col.data_offset);
    //     // for (uint32_t i = idx.size(); i<col.count; ++i) {
    //     //     std::string val = std::string(get_ptr<char>(offsets[i]));
    //     //     std::string token = normalize_token(val, 200);
    //     //     idx[val].push_back(token);
    //     // }
    //     // indexes[index_name] = idx;
    // }

    // void CreateTGIndex(const std::string& col_name) {
    //     if (column_map.find(col_name) == column_map.end()) {
    //         throw std::runtime_error("Column not found: " + col_name);
    //     }

    //     if (HasTGIndex(col_name)) {
    //         return;
    //     }

    //     int col_idx = column_map[col_name];
    //     ColumnHeader& col = columns[col_idx];
    //     if (col.type != ColType::TEXT) {
    //         throw std::runtime_error("Index only supported on TEXT columns currently");
    //     }

    //     // (const std::string& name, ColType type, uint32_t initial_capacity, int dim = 0)


    //     const std::string index_name = col_name + "_tgk";
    //     BigTable* t = CreateTable("systems"); 
    //     t->CreateGroup("systems", index_name, size_mb, "Index-Data for column " + col_name);
    //     ColumnarTiger* tg = t->GetGroup(index_name);
    //     tg->CreateColumn("term", ColType::TEXT, col.capacity);
    //     tg->CreateColumn("rowid", ColType::TEXT, col.capacity);
    //     tg->CreateColumn("count", ColType::TEXT, col.capacity);
    // }

    void CreateIndex(const std::string& col_name) {
        if (column_map.find(col_name) == column_map.end()) {
            throw std::runtime_error("Column not found: " + col_name);
        }
        if (HasIndex(col_name)) {
            return;            
        }

        int col_idx = column_map[col_name];
        ColumnHeader& col = columns[col_idx];
        if (col.type != ColType::TEXT) {
            throw std::runtime_error("Index only supported on TEXT columns currently");
        }

        // Rebuild index
        std::unordered_map<std::string, std::vector<RowID>> idx;
        RelPtr* offsets = get_ptr<RelPtr>(col.data_offset);
        const auto index_size = col.count;
        for (uint32_t i = 0; i<index_size; ++i) {
            // if (offsets[i] == DELETED_FLAG) continue;
            std::string val = std::string(get_ptr<char>(offsets[i]));
            // tokenize val, remove tokens
            // stem
            // lookup dictionary
            // tokens =
            idx[val].push_back(i);
        }

        indexes[col_name] = idx;
    }

    void CreateSecondaryIndex(const std::string& col_name) {
        // 1. Validation
        if (column_map.find(col_name) == column_map.end()) {
            throw std::runtime_error("Column not found: " + col_name);
        }

        // Check if index already exists
        const std::string index_name = col_name + "_sk";
        if (sk_indexes.find(index_name) != sk_indexes.end()) {
            return; 
        }

        int col_idx = column_map[col_name];
        ColumnHeader& col = columns[col_idx];

        if (col.type != ColType::TEXT) {
            throw std::runtime_error("Secondary Index only supported on TEXT columns currently");
        }

        // 2. Define Index Scope (Partial Indexing Strategy)
        // Only index the first 43% of rows (as requested)
        // For updates, we will track this limit.
        const uint32_t index_limit = static_cast<uint32_t>(col.count * 0.43);

        // 3. Build Index
        IndexData idx_data;
        idx_data.last_indexed_row = 0;
        
        RelPtr* offsets = get_ptr<RelPtr>(col.data_offset);

        for (uint32_t i = 0; i < index_limit; ++i) {
            std::string raw_val = std::string(get_ptr<char>(offsets[i]));
            std::string token = normalize_token(raw_val, MAX_SK_CHUNK_SIZE);
            if (!token.empty()) {
                idx_data.map[token].push_back(i);
            }
        }

        // Track where we stopped
        idx_data.last_indexed_row = index_limit;

        // 4. Store Index
        sk_indexes[index_name] = std::move(idx_data);
        std::cout << "Secondary Index '" << index_name << "' created on " 
                  << index_limit << "/" << col.count << " rows.\n";
    }

    void ClearSecondaryIndex(const std::string& col_name) {
        const std::string index_name = col_name + "_sk";
        if (column_map.find(col_name) == column_map.end()) {
            throw std::runtime_error("Column not found: " + col_name);
        }

        if (sk_indexes.find(index_name) == sk_indexes.end()) {
            // Option: Auto-create if missing, or just return
            return; 
        }

        IndexData& idx_data = sk_indexes[index_name];

        idx_data.last_indexed_row=0;
        idx_data.map.clear();
    }

    void UpdateSecondaryIndex(const std::string& col_name) {
        const std::string index_name = col_name + "_sk";

        // 1. Validate Column & Index Existence
        if (column_map.find(col_name) == column_map.end()) {
            throw std::runtime_error("Column not found: " + col_name);
        }
        if (sk_indexes.find(index_name) == sk_indexes.end()) {
            // Option: Auto-create if missing, or just return
            return; 
        }

        // 2. Retrieve Engine State
        int col_idx = column_map[col_name];
        ColumnHeader& col = columns[col_idx];
        IndexData& idx_data = sk_indexes[index_name];

        // 3. Check for work
        // Use stored watermark, NOT map size
        if (idx_data.last_indexed_row >= col.count) {
            return; // Up to date
        }

        const uint32_t index_limit = static_cast<uint32_t>(col.count * 0.13);
        uint32_t chunk_rest = idx_data.last_indexed_row+index_limit;

        std::cout << "Updating index '" << index_name << "' from row " 
                  << idx_data.last_indexed_row << " to " << chunk_rest << "...\n";

        if (chunk_rest>=col.count) {
            return;
        }

        // 4. Incremental Update Loop
        RelPtr* offsets = get_ptr<RelPtr>(col.data_offset);

        // Start exactly where we left off
        uint32_t count_ins = 0;
        for (uint32_t i = idx_data.last_indexed_row; i<chunk_rest; ++i) {
            std::string raw_val = std::string(get_ptr<char>(offsets[i]));
            // Apply same normalization logic
            std::string token = normalize_token(raw_val, MAX_SK_CHUNK_SIZE);
            // Advanced: Lookup dictionary / stemming here if needed
            if (!token.empty()) {
                idx_data.map[token].push_back(i);
                count_ins += 1;
            }
        }

        // 5. Update Watermark
        idx_data.last_indexed_row += count_ins;
    }

    bool HasIndex(const std::string& col_name) {
        return indexes.find(col_name) != indexes.end();
    }

    bool HasSKIndex(const std::string& col_name) {
        return sk_indexes.find(col_name) != sk_indexes.end();
    }

    std::vector<RowID> LookupIndex(const std::string& col_name, const std::string& op, const std::string& val, int limit) {
        // 1. Validate index exists
        if (indexes.find(col_name) == indexes.end()) {
            return {}; 
        }

        auto& idx = indexes[col_name];
        const std::string& OP=str_to_lower(op);

        // 2. Try Exact Match (Fast path)
        // if ((OP=="=" || OP=="eq") && idx.find(val) != idx.end()) {
        //     return idx[val];
        // }
        if (OP == "eq" || OP == "=") {
            auto it_ids = idx.find(val);
            if (it_ids != idx.end()) {
                if (limit > 0 && it_ids->second.size() > (size_t)limit) {
                    return std::vector<RowID>(it_ids->second.begin(), it_ids->second.begin() + limit);
                }
                return it_ids->second;
            }
            return {};
        }

        // 3. Fallback: Scan keys if index cardinality is small (< 10000)
        // This handles "contains" logic on keys without full table scan
        if ( (OP=="contains" || OP=="not_contains" || OP=="ilike" || OP=="like"||OP=="starts_with"||OP=="ends_with")) {
            const auto stop = 0.7 * idx.size();
            uint32_t counter_add = 0;
            std::vector<RowID> fuzzy_results;
            for (const auto& pair : idx) {
                if (counter_add>=stop || (limit > 0 && fuzzy_results.size() >= (size_t)limit)) {
                    break;
                }

                bool match = false;
                const std::string& key = pair.first;
                // Note: Keys in map are already normalized (lowercase, no spaces)
                // So 'contains' behaves like 'ilike' automatically on the normalized key space.
                if (OP == "contains" || OP == "like" || OP == "ilike") {
                     if (key.find(val) != std::string::npos) match = true;
                }
                // else if (OP == "not_contains") {
                //      if (key.find(val) == std::string::npos) match = true;
                // }
                // else if (OP == "!=" || OP == "NEQ") {
                //      if (key!=val && pair.second.size()>0) match = true;
                // }
                else if (OP == "starts_with") {
                     if (str_starts_with(key, val)) match = true;
                }
                else if (OP == "ends_with") {
                     if (str_ends_with(key, val)) match = true;
                }

                if (match) {
                     for(RowID r : pair.second) {
                        if (fuzzy_results.size() >= (size_t)limit) {
                            break;
                        }
                        fuzzy_results.push_back(r);
                        if (limit > 0 && fuzzy_results.size() >= (size_t)limit) {
                            break;
                        }
                     }
                }

                counter_add += 1;
            }
            return fuzzy_results;
        }

        return {};
    }

    std::vector<RowID> LookupSKIndex(const std::string& col_name, const std::string& op, const std::string& val, int limit) {
        const std::string index_name = col_name+"_sk";

        auto it_idx = sk_indexes.find(index_name);
        if (it_idx == sk_indexes.end()) {
            return {}; // Index doesn't exist
        }
        const std::string& OP=str_to_lower(op);
        std::string token = normalize_token(val, MAX_SK_CHUNK_SIZE);
        if (token.empty()) return {};

        const auto& idx_data = it_idx->second; // Access IndexData
        std::vector<RowID> fuzzy_results;

        if ((OP=="=" || OP=="EQ") ) {
            auto it_ids = idx_data.map.find(token);
            if (it_ids != idx_data.map.end()) {
                return it_ids->second;
            }
        }
        // else if (OP=="!=" || OP=="NEQ" ) {
        //     for (const auto& pair : idx_data.map) {
        //         const std::string& key = pair.first;
        //         if (limit > 0 && fuzzy_results.size() >= (size_t)limit) {
        //             break;
        //         }

        //         if (key!=val && (key!=val && pair.second.size()>0)) { // TODO: could it be this simple ???
        //             for(RowID r : pair.second) {
        //                 if (fuzzy_results.size() >= (size_t)limit) {
        //                     break;
        //                 }

        //                 fuzzy_results.push_back(r);
        //                 if (limit > 0 && fuzzy_results.size() >= (size_t)limit) {
        //                     break;
        //                 }
        //             }
        //         }
        //     }

        // } 
        // else if (idx_data.map.size() < 100000 && (OP=="not_contains"||OP=="contains"||OP=="ilike"||OP=="like"||OP=="starts_with"||OP=="ends_with")) {
        else if ((OP=="not_contains"||OP=="contains"||OP=="ilike"||OP=="like"||OP=="starts_with"||OP=="ends_with")) {
            uint32_t counter_add = 0;
            const auto stop = 0.7 * idx_data.map.size();
            for (const auto& pair : idx_data.map) {
                if (counter_add>=stop || (limit > 0 && fuzzy_results.size() >= (size_t)limit)) {
                    break;
                }

                bool match = false;
                const std::string& key = pair.first;
                // Note: Keys in map are already normalized (lowercase, no spaces)
                // So 'contains' behaves like 'ilike' automatically on the normalized key space.
                if (OP == "contains" || OP == "like" || OP == "ilike") {
                     if (key.find(token) != std::string::npos) match = true;
                }
                else if (OP == "starts_with") {
                     if (str_starts_with(key, token)) match = true;
                }
                // else if (OP == "not_contains") {
                //      if (key.find(val) == std::string::npos) match = true;
                // }
                else if (OP == "ends_with") {
                     if (str_ends_with(key, token)) match = true;
                }
                if (match) {
                     for(RowID r : pair.second) {
                        if (fuzzy_results.size() >= (size_t)limit) {
                            break;
                        }
                        fuzzy_results.push_back(r);
                        if (limit > 0 && fuzzy_results.size() >= (size_t)limit) {
                            break;
                        }
                     }
                }

                counter_add += 1;
                // if (OP=="contains" || OP=="like") {
                //     if (pair.first.find(token) != std::string::npos) {
                //          fuzzy_results.insert(fuzzy_results.end(), pair.second.begin(), pair.second.end());
                //     }
                // }
                // else if (OP=="ilike") {
                //     if (pair.first.find(token) != std::string::npos 
                //         || str_to_lower(pair.first).find(str_to_lower(token))!= std::string::npos) {
                //          fuzzy_results.insert(fuzzy_results.end(), pair.second.begin(), pair.second.end());
                //     }
                // }
                // else if (OP=="starts_with") {
                //     if (str_starts_with(pair.first, token)) {
                //          fuzzy_results.insert(fuzzy_results.end(), pair.second.begin(), pair.second.end());
                //     }
                // }
                // else if (OP=="ends_with") {
                //     if (str_ends_with(pair.first, token)) {
                //          fuzzy_results.insert(fuzzy_results.end(), pair.second.begin(), pair.second.end());
                //     }
                // }
            }
            return fuzzy_results;
        }
        
        return fuzzy_results;
    }

    // std::unordered_map<std::string, std::string> BuildSerializedIndex(const std::string& col_name, uint32_t limit) {
    //     if (!HasColumn(col_name)) throw std::runtime_error("Column not found");
    //     if (GetColumnType(col_name) != ColType::TEXT) throw std::runtime_error("Only TEXT supported for TigerIndex");
        
    //     std::unordered_map<std::string, std::vector<RowID>> raw_idx;
    //     int idx = column_map[col_name];
    //     RelPtr* offsets = get_ptr<RelPtr>(columns[idx].data_offset);
    //     uint32_t count = std::min(limit, columns[idx].count);
        
    //     for(uint32_t i=0; i<count; ++i) {
    //          // if (offsets[i] == DELETED_FLAG) continue;
    //          std::string val = std::string(get_ptr<char>(offsets[i]));
    //          raw_idx[val].push_back(i);
    //     }
        
    //     std::unordered_map<std::string, std::string> serialized;
    //     for(auto& p : raw_idx) {
    //         std::stringstream ss;
    //         ss << "[";
    //         for(size_t k=0; k<p.second.size(); ++k) {
    //             ss << p.second[k] << (k < p.second.size()-1 ? "," : "");
    //         }
    //         ss << "]";
    //         serialized[p.first] = ss.str();
    //     }
    //     return serialized;
    // }

    std::unordered_map<std::string, std::vector<RowID>> BuildIndexMap(const std::string& col_name, uint32_t start_row, uint32_t limit_or_end) {
        if (!HasColumn(col_name)) throw std::runtime_error("Column not found");
        if (GetColumnType(col_name) != ColType::TEXT) throw std::runtime_error("Only TEXT supported");
        
        std::unordered_map<std::string, std::vector<RowID>> raw_idx;
        int idx = column_map[col_name];
        RelPtr* offsets = get_ptr<RelPtr>(columns[idx].data_offset);
        uint32_t count = columns[idx].count;
        uint32_t actual_end = std::min(count, start_row + limit_or_end);
        
        for(uint32_t i=start_row; i<actual_end; ++i) {
             // if (offsets[i] == DELETED_FLAG) continue;
             std::string val = std::string(get_ptr<char>(offsets[i]));
             if (trim(val)=="") {
                continue;
             }
             raw_idx[val].push_back(i);
        }
        return raw_idx;
    }

    std::unordered_map<std::string, std::string> BuildSerializedIndex(const std::string& col_name, uint32_t limit) {
         // Uses BuildIndexMap with 0 start and limit
         auto raw = BuildIndexMap(col_name, 0, limit);
         std::unordered_map<std::string, std::string> serialized;
         for(auto& p : raw) {
            serialized[p.first] = SerializeIDs(p.second);
         }
         return serialized;
    }

    static std::string SerializeIDs(const std::vector<RowID>& ids) {
        std::stringstream ss;
        ss << "[";
        for(size_t k=0; k<ids.size(); ++k) {
            ss << ids[k] << (k < ids.size()-1 ? "," : "");
        }
        ss << "]";
        return ss.str();
    }
    
    // Helper to deserialize ID list
    static std::vector<RowID> DeserializeIDs(const std::string& s) {
        std::vector<RowID> ids;
        if (s.length() < 2) return ids;
        std::string inner = s.substr(1, s.length()-2);
        std::stringstream ss(inner);
        std::string item;
        while(std::getline(ss, item, ',')) {
            if(!item.empty()) ids.push_back(std::stoi(item));
        }
        return ids;
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
        
        // Update Index if exists (Naive: append only)
        if (indexes.find(col_name) != indexes.end()) {
            indexes[col_name][text].push_back(row);
        }
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
    
    // --- New Feature: SELECT_FROM_ROWS ---
    std::string SelectFromRows(const std::string& col_name, const std::vector<RowID>& rows) {
        if (column_map.find(col_name) == column_map.end()) throw std::runtime_error("Column not found: " + col_name);
        int idx = column_map[col_name];
        ColType type = columns[idx].type;
        
        std::stringstream ss;
        ss << "[";
        for (size_t i = 0; i < rows.size(); ++i) {
            RowID r = rows[i];
            if (type == ColType::TEXT) ss << "\"" << GetText(col_name, r) << "\"";
            else if (type == ColType::INT32) ss << GetInt(col_name, r);
            else if (type == ColType::FLOAT32) ss << GetFloat(col_name, r);
            else ss << "\"[Vector]\""; // Simplified
            
            if (i < rows.size() - 1) ss << ", ";
        }
        ss << "]";
        return ss.str();
    }

    // Helper for JOIN (Internal)
    // Returns value as string for dynamic comparison/lookup
    std::string GetValueAsString(const std::string& col_name, RowID row) {
        int idx = column_map.at(col_name);
        ColType type = columns[idx].type;
        if (type == ColType::TEXT) return GetText(col_name, row);
        if (type == ColType::INT32) return std::to_string(GetInt(col_name, row)); // Careful with float comparison strings
        return "";
    }
    
    uint32_t GetRowCount(const std::string& col_name) {
        if (column_map.find(col_name) == column_map.end()) throw std::runtime_error("Column not found: " + col_name);
        return columns[column_map.at(col_name)].count;
    }

    size_t GetCapacityBytes() const { return capacity_bytes; }
    uint32_t GetColumnCapacity(const std::string& col_name) {
        if (column_map.find(col_name) == column_map.end()) throw std::runtime_error("Column not found: " + col_name);
        return columns[column_map.at(col_name)].capacity;
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

    std::vector<RowID> FilterIndex(const std::string& col_name, const std::string& op_raw, const std::string& val_str, int limit) {
        if (column_map.find(col_name) == column_map.end()) {
            throw std::runtime_error("Column not found: " + col_name);
        }

        int col_idx = column_map[col_name];
        ColumnHeader& col = columns[col_idx];
        
        std::string op = str_to_upper(op_raw);
        std::vector<RowID> matches;
        matches.reserve(limit > 0 ? limit : 128); 

        if (op == "IN") {
            throw std::runtime_error("operator IN not supported in FilterIndex");
        }

        std::vector<RowID> candidates;
        if (HasIndex(col_name)) {
            candidates = LookupIndex(col_name, op, val_str, limit);
            if (limit > 0 && candidates.size() > (size_t)limit) {
                candidates.resize(limit);
            }
        }

        return candidates;
    }

    std::vector<RowID> FilterSecondaryIndex(const std::string& col_name, const std::string& op_raw, const std::string& val_str, int limit = -1) {
        if (column_map.find(col_name) == column_map.end()) {
            throw std::runtime_error("Column not found: " + col_name);
        }

        int col_idx = column_map[col_name];
        ColumnHeader& col = columns[col_idx];

        if (limit<0) {
            limit=10;
        }

        std::string op = str_to_upper(op_raw);
        std::vector<RowID> matches;
        matches.reserve(limit > 0 ? limit : 128); 

        if (op == "IN") {
            throw std::runtime_error("operator IN not supported in FilterSecondaryIndex");
        }

        const std::string index_name = col_name + "_sk";

        std::vector<RowID> candidates;
        if (HasSKIndex(index_name)) {
            candidates = LookupSKIndex(col_name, op, val_str, limit);
            if (limit > 0 && candidates.size() > (size_t)limit) {
                candidates.resize(limit);
            }
        }

        return candidates;
    }

    std::vector<RowID> Filter(const std::string& col_name, const std::string& op_raw, const std::string& val_str, int limit=10) {
        if (column_map.find(col_name) == column_map.end()) {
            throw std::runtime_error("Column not found: " + col_name);
        }

        int col_idx = column_map[col_name];
        ColumnHeader& col = columns[col_idx];
        
        std::string op = str_to_upper(op_raw);
        std::vector<RowID> matches;
        matches.reserve(limit > 0 ? limit : 128); 

        // Handle IN operator setup
        bool is_in_op = (op == "IN");
        std::unordered_set<int32_t> int_set;
        std::vector<float> float_list;
        std::unordered_set<std::string> text_set;

        if (is_in_op) {
             std::vector<std::string> raw_list = parse_in_list(val_str);
             if (col.type == ColType::INT32) {
                 for(const auto& s : raw_list) { try { int_set.insert(std::stoi(s)); } catch(...) {} }
             } else if (col.type == ColType::FLOAT32) {
                 for(const auto& s : raw_list) { try { float_list.push_back(std::stof(s)); } catch(...) {} }
             } else if (col.type == ColType::TEXT) {
                 for(const auto& s : raw_list) text_set.insert(s);
             }
        }

        // Use Index if available for EQ
        // if (!is_in_op && (op == "EQ" || op == "=") && HasIndex(col_name)) {
        if (!is_in_op && HasIndex(col_name)) {
             std::vector<RowID> candidates = LookupIndex(col_name, op, val_str, limit);
             if (limit > 0 && candidates.size() > (size_t)limit) candidates.resize(limit);
             // if (candidates.size()>0) {
                 return candidates;
             // }
        }

        if (col.type == ColType::INT32) {
            int32_t target = 0;
            if (!is_in_op) target = std::stoi(val_str);
            int32_t* data = get_ptr<int32_t>(col.data_offset);
            for (uint32_t i = 0; i < col.count; ++i) {
                if (limit > 0 && matches.size() >= (size_t)limit) break;
                bool match = false;
                if (is_in_op) match = (int_set.find(data[i]) != int_set.end());
                else if (op == "EQ" || op == "=") match = (data[i] == target);
                else if (op == "NEQ" || op == "!=") match = (data[i] != target);
                else if (op == "GTE" || op == ">=") match = (data[i] >= target);
                else if (op == "LTE" || op == "<=") match = (data[i] <= target);
                else if (op == "GT" || op == ">") match = (data[i] > target);
                else if (op == "LT" || op == "<") match = (data[i] < target);
                if (match) matches.push_back(i);
            }
        }
        else if (col.type == ColType::FLOAT32) {
            float target = 0.0f;
            if (!is_in_op) target = std::stof(val_str);
            float* data = get_ptr<float>(col.data_offset);
            for (uint32_t i = 0; i < col.count; ++i) {
                if (limit > 0 && matches.size() >= (size_t)limit) break;
                bool match = false;
                if (is_in_op) {
                     for(float f : float_list) { 
                         if (std::abs(data[i] - f) < 1e-6) { match = true; break; } 
                     }
                }
                else if (op == "EQ" || op == "=") match = (std::abs(data[i] - target) < 1e-6); 
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
                
                if (is_in_op) match = (text_set.find(val) != text_set.end());
                else if (op == "EQ" || op == "=") match = (val == val_str);
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
        if (column_map.find(col_name) == column_map.end()) {
            throw std::runtime_error("Column not found: " + col_name);
        }
        return columns[column_map.at(col_name)].count;
    }

    std::string GetStats() {
        std::stringstream ss;
        ss << "   Arena Used: " << head << " / " << capacity_bytes << " bytes\n";
        ss << "   Columns: " << columns.size() << "\n";
        for(const auto& col : columns) {
            auto type = col.type;
            std::string typeL = "";
            switch(type) {
                case ColType::INT32: typeL="INT32"; break;
                case ColType::FLOAT32: typeL="FLOAT"; break;
                case ColType::TEXT: typeL="TEXT"; break;
                case ColType::VECTOR_F32: typeL="VECTOR["+ std::to_string(col.vector_dim) + "]"; break;
            }
            ss << "    - " << col.name << " Type:" << typeL << " (Rows: " << col.count << ", Cap: " << col.capacity << ")\n";
        }

        ss << "   Indexes: " << indexes.size() << "\n";
        for(const auto& entry : indexes) {
            ss << "    *> Name: " << entry.first << ", Count: " << entry.second.size() << "\n";
        }

        ss << "   SK-Indexes: " << sk_indexes.size() << "\n";
        for(const auto& entry : sk_indexes) {
            ss << "    >> Name: " << entry.first 
               << ", Keys: " << entry.second.map.size() 
               << ", Scanned: " << entry.second.last_indexed_row << "\n";
        }

        return ss.str();
    }
    
    // Check if column type is string/text for Join
    bool IsText(const std::string& col_name) {
         if (column_map.find(col_name) == column_map.end()) return false;
         return columns[column_map.at(col_name)].type == ColType::TEXT;
    }
};

// --- ARCHITECTURE ---

struct ColumnGroup {
    std::unique_ptr<ColumnarTiger> engine;
    std::string role; 
    ColumnGroup(size_t size_mb, std::string r) : engine(std::make_unique<ColumnarTiger>(size_mb)), role(r) {}
    ColumnGroup(std::unique_ptr<ColumnarTiger> eng, std::string r) : engine(std::move(eng)), role(r) {}
    void Reset() { engine->Reset(); }
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
    bool GroupExists(const std::string& group_name) {
        return (groups.find(group_name) != groups.end());
    }
    void Reset() { for (auto& pair : groups) pair.second->Reset(); }
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

    std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(' ');
        if (std::string::npos == first) return str;
        size_t last = str.find_last_not_of(' ');
        return str.substr(first, (last - first + 1));
    }

    void CreateTigerIndex(const std::string& table, const std::string& group, const std::string& col) {
         // 1. Get Source
         BigTable* t = GetTable(table);
         ColumnarTiger* src = t->GetGroup(group);
         const float MAX_INDEX_SIZE = 0.7;
         const float START_SCAN_SIZE = 0.4;

         // 3. Create System Table (if not exists)
         BigTable* sys = CreateTable("systems");

         // 4. Create Index Group: idx_{table}_{group}_{col}
         std::string idx_group_name = "idx_" + table + "_" + group + "_" + col;
         if (sys->GroupExists(idx_group_name)) {
            return;
         }

         uint32_t total_rows = src->GetRowCount(col);
         uint32_t scan_limit = (uint32_t)(total_rows * START_SCAN_SIZE);

         auto index_map = src->BuildSerializedIndex(col, scan_limit);

         ColumnarTiger* idx_engine = sys->CreateGroup(idx_group_name, 564, "index");

         // Auto-sizing: 90% of source group capacity (converted back to MB)
         size_t src_mb = src->GetCapacityBytes() / (1024 * 1024);
         size_t idx_mb = std::max((size_t)164, (size_t)(src_mb * MAX_INDEX_SIZE));

         // 5. Schema & Populate
         // idx_engine->CreateColumn("term", ColType::TEXT, index_map.size() + 1000);
         // idx_engine->CreateColumn("ids", ColType::TEXT, index_map.size() + 1000);
         // idx_engine->CreateColumn("count", ColType::INT32, index_map.size() + 1000);
         uint32_t src_col_cap = src->GetColumnCapacity(col);
         uint32_t target_cap = (uint32_t)(src_col_cap * MAX_INDEX_SIZE);
         uint32_t final_cap = std::max(target_cap, (uint32_t)(index_map.size() + 1000));

         idx_engine->CreateColumn("term", ColType::TEXT, final_cap);
         idx_engine->CreateColumn("ids", ColType::TEXT, final_cap);
         idx_engine->CreateColumn("count", ColType::INT32, final_cap);

         for(auto& p : index_map) {
            const auto content = trim(p.first);
            if (content.size()==0 || p.second.size()==0 || p.second=="[]") {
                continue;
            }

            RowID r = idx_engine->AddRow();
            idx_engine->SetText("term", r, content);
            idx_engine->SetText("ids", r, p.second);

            // Count parsing heuristic (count commas + 1)
            int c = 1;
            for(char ch : p.second) if(ch==',') c++;
            if(p.second == "[]") c=0;
            idx_engine->SetInt("count", r, c); 
         }

         idx_engine->SetWatermark(scan_limit);
     }

    void UpdateTigerIndex(const std::string& table, const std::string& group, const std::string& col) {
         BigTable* sys = GetTable("systems");
         std::string idx_group_name = "idx_" + table + "_" + group + "_" + col;
         ColumnarTiger* idx_engine = sys->GetGroup(idx_group_name);
         
         BigTable* t = GetTable(table);
         ColumnarTiger* src = t->GetGroup(group);
         
         uint32_t start = idx_engine->GetWatermark();
         uint32_t current_total = src->GetRowCount(col);
         
         // Update by 13% of total content, or up to max
         // 13% logic as requested (simulation of partial update)
         uint32_t increment = std::max(1u, (uint32_t)(current_total * 0.13));
         uint32_t end = std::min(current_total, start + increment);
         
         if (start >= end) return; // Nothing to update
         
         // Build partial map for new rows
         auto new_map = src->BuildIndexMap(col, start, end);

         // Merge into index
         // Since idx_engine stores term -> ids, we need to find existing terms
         // Optimized: idx_engine supports internal hash index if we call CreateIndex("term")
         // But for now we might have to scan or rely on idx_engine's Filter.
         // Let's assume idx_engine has an internal index on "term" for speed?
         // We can force create it:
         // if (!idx_engine->HasIndex("term")) {
         //    idx_engine->CreateIndex("term");
         // }

         for (auto& p : new_map) {
            // LookupIndex(const std::string& col_name, const std::string& op, const std::string& val, int limit)
             auto existing_rows = idx_engine->LookupIndex("term", "=", p.first, (int)increment);
             if (existing_rows.empty()) {
                 // New term
                const auto content = trim(p.first);
                if (content.size()==0 || p.second.size()==0) {
                    continue;
                }

                 RowID r = idx_engine->AddRow();
                 idx_engine->SetText("term", r, content);
                 idx_engine->SetText("ids", r, ColumnarTiger::SerializeIDs(p.second));
                 idx_engine->SetInt("count", r, p.second.size());
             } else {
                 // Update existing
                 RowID r = existing_rows[0];
                 std::string existing_ids_str = idx_engine->GetText("ids", r);
                 std::vector<RowID> current_ids = ColumnarTiger::DeserializeIDs(existing_ids_str);
                 
                 // Append new IDs
                 current_ids.insert(current_ids.end(), p.second.begin(), p.second.end());
                 
                 idx_engine->SetText("ids", r, ColumnarTiger::SerializeIDs(current_ids));
                 idx_engine->SetInt("count", r, current_ids.size());
             }
         }
         
         idx_engine->SetWatermark(end);
    }

    // FilterTigerIndex(table_name, group, col_name, op, val, limit)
    // std::vector<RowID> FilterTigerIndex(const std::string& table, const std::string& group, 
    //     const std::string& col, const std::string& op, const std::string& val, int limit) {
    //     if (tables.find("systems") == tables.end()) {
    //         return {};
    //     };

    //     BigTable* sys = tables["systems"].get();
    //     std::string idx_group_name = "idx_" + table + "_" + group + "_" + col;
    //     if (!sys->GroupExists(idx_group_name)) {
    //         return {};
    //     }

    //     ColumnarTiger* idx_engine = sys->GetGroup(idx_group_name);
    //     auto matches = idx_engine->Filter("term", op, val, limit);

    //     std::vector<RowID> result;
    //     for (std::vector<RowID>::iterator i = matches.begin(); i != matches.end(); ++i) {
    //         if (result.size() >= limit) {
    //             break;
    //         }
    //         result.push_bash(std::stoi(idx_engine->GetText("ids", i))); // parse GetText to rowid
    //     }
    //     return result;
    // }

    std::vector<RowID> FilterTigerIndex(const std::string& table, const std::string& group, 
        const std::string& col, const std::string& op, const std::string& val, int limit) {
        
        if (tables.find("systems") == tables.end()) return {};

        BigTable* sys = tables["systems"].get();
        std::string idx_group_name = "idx_" + table + "_" + group + "_" + col;
        
        if (!sys->GroupExists(idx_group_name)) {
            return {};
        }

        ColumnarTiger* idx_engine = sys->GetGroup(idx_group_name);
        // 1. Find matching terms in the index (e.g. term CONTAINS "tech")
        auto matches = idx_engine->Filter("term", op, val, limit);

        std::vector<RowID> result;
        for (auto r : matches) {
            if (limit > 0 && result.size() >= (size_t)limit) break;
            // 2. Get the list of IDs for this term
            std::string serialized = idx_engine->GetText("ids", r);
            std::vector<RowID> ids = ColumnarTiger::DeserializeIDs(serialized);
            // 3. Accumulate
            for(auto id : ids) {
                result.push_back(id);
                if (limit > 0 && result.size() >= (size_t)limit) break;
            }
        }
        return result;
    }

    // --- JOIN Implementation ---
    // JOIN(table, group_a, col_a, group_b, col_b, op, val)
    // Returns List of RowIDs from Group A that match.
    // 1. Filter Group B based on (op, val) -> Get IDs_B
    // 2. For each ID in IDs_B, get Value_B (from col_b)
    // 3. Use Value_B to query Group A (using index on col_a if avail, or scan) -> IDs_A
    std::vector<RowID> JoinFilter(const std::string& table, const std::string& g_a, const std::string& c_a,
                                  const std::string& g_b, const std::string& c_b,
                                  const std::string& op, const std::string& val) {
        
        BigTable* t = GetTable(table);
        ColumnarTiger* eng_a = t->GetGroup(g_a);
        ColumnarTiger* eng_b = t->GetGroup(g_b);

        // 1. Filter Group B
        std::vector<RowID> matches_b = eng_b->Filter(c_b, op, val);
        std::vector<RowID> results_a;

        // 2. Iterate matches and join to A
        for (RowID id_b : matches_b) {
            std::string join_val = eng_b->GetValueAsString(c_b, id_b); // Currently assumes joining on TEXT
            // Only works if col_b is TEXT for now (since GetValueAsString is simplified)
            // Real implementation needs polymorphic handling
            
            // 3. Find in A
            // Ideal: eng_a->LookupIndex(c_a, join_val) if c_a is indexed
            if (eng_a->HasIndex(c_a)) {
                auto hits = eng_a->LookupIndex(c_a, "=", join_val, 100); // TODO: receive limit here
                results_a.insert(results_a.end(), hits.begin(), hits.end());
            } else {
                // Scan A (Slow join)
                auto hits = eng_a->Filter(c_a, "EQ", join_val);
                results_a.insert(results_a.end(), hits.begin(), hits.end());
            }
        }
        return results_a;
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
        const std::string ffl = dir_path + "_manifest.txt";
        std::ifstream manifest(ffl);
        if (!manifest) {
            throw std::runtime_error("Manifest("+ffl+") not found");
        }

        tables.clear();
        std::string t_name, g_name, role;
        while (manifest >> t_name >> g_name >> role) {
            BigTable* t = CreateTable(t_name); 
            std::string filename = dir_path + "_" + t_name + "_" + g_name + ".tg";
            auto loaded_engine = ColumnarTiger::Load(filename);
            t->groups[g_name] = std::make_unique<ColumnGroup>(std::move(loaded_engine), role);
        }
    }
    void Reset() { for (auto& pair : tables) pair.second->Reset(); }
    std::string GetStats() {
        std::stringstream ss;
        ss << "\n=== NativeMetaDB Statistics ===\n";
        ss << "Total Tables: " << tables.size() << "\n";
        for (const auto& pair : tables) ss << pair.second->GetStats();
        ss << "===============================\n";
        return ss.str();
    }
};

// --- TGQL ---
struct ExecutionResult {
    std::string msg;
    std::vector<RowID> rows;
    bool has_rows = false;
    ColumnarTiger* context_engine = nullptr; 
};

class TGQL {
    // ... [Parsing Helpers same as before] ...
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

    static ExecutionResult RunInternal(NativeMetaDB& db, std::string cmd, std::vector<std::string> args) {
        ExecutionResult res;
        // ... [CREATE/ADD/UPDATE/GET logic same as before] ...
        const uint32_t MAX_OUTPUT_SIZE = 6000;
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
            tg->CreateColumn(args[2], type, 10000, dim); 
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
            res.msg = std::to_string(rid);
        }
        else if (cmd == "UPDATE" || cmd == "SET") {
            if (args.size() < 5) throw std::runtime_error("Req: table, group, col, row_id, val");
            BigTable* t = db.GetTable(args[0]);
            ColumnarTiger* tg = t->GetGroup(args[1]);
            std::string col = args[2];
            RowID rid = std::stoi(args[3]);
            std::string val = args[4];
            auto &colRef = tg->columns[tg->column_map[col]];

            if (colRef.type == ColType::INT32) {
                tg->SetInt(col, rid, std::stoi(val));

            } else if (colRef.type == ColType::FLOAT32)  {
                tg->SetFloat(col, rid, std::stoi(val));

            } else if (colRef.type == ColType::TEXT) {
                tg->SetText(col, rid, val);

            } else if (colRef.type == ColType::VECTOR_F32) {
                tg->SetVector(col, rid, parse_vector_data(val));
            } else {
                throw std::runtime_error("Unexpected Type on col "+col);
            }

            res.msg = "Updated.";
        }
        else if (cmd == "GET") {
            if (args.size() < 4) throw std::runtime_error("Req: table, group, col, row_id");
            BigTable* t = db.GetTable(args[0]);
            ColumnarTiger* tg = t->GetGroup(args[1]);
            ColType type = tg->GetColumnType(args[2]);
            RowID rowid = std::stoi(args[3]);
            // if (rowid >= col.count) {
            //     throw std::runtime_error("rowid outside valid range");                
            // }
            if (type == ColType::TEXT) res.msg = tg->GetText(args[2], std::stoi(args[3]));
            else if (type == ColType::INT32) res.msg = std::to_string(tg->GetInt(args[2], std::stoi(args[3])));
            else if (type == ColType::FLOAT32) res.msg = std::to_string(tg->GetFloat(args[2], std::stoi(args[3])));
            else res.msg = "[VECTOR]";
        }
        
        // --- NEW FEATURES ---
        else if (cmd == "CLEAR_INDEX") {
            if (args.size() < 3) throw std::runtime_error("Req: table, group, col");
            BigTable* t = db.GetTable(args[0]);
            ColumnarTiger* tg = t->GetGroup(args[1]);
            tg->ClearIndex(args[2]);
            res.msg = "Index cleared on " + args[2];
        }
        else if (cmd == "CREATE_INDEX") {
            if (args.size() < 3) throw std::runtime_error("Req: table, group, col");
            BigTable* t = db.GetTable(args[0]);
            ColumnarTiger* tg = t->GetGroup(args[1]);
            tg->CreateIndex(args[2]);
            res.msg = "Index created on " + args[2];
        }
        else if (cmd == "CREATE_SK_INDEX") {
            if (args.size() < 3) throw std::runtime_error("Req: table, group, col");
            BigTable* t = db.GetTable(args[0]);
            ColumnarTiger* tg = t->GetGroup(args[1]);
            tg->CreateSecondaryIndex(args[2]);
            res.msg = "Secondary-Index created on " + args[2];
        }
        else if (cmd == "CREATE_TG_INDEX") {
            if (args.size() < 3) throw std::runtime_error("Req: table, group, col");

            BigTable* t = db.GetTable(args[0]);
            ColumnarTiger* tg = t->GetGroup(args[1]);
            const std::string &table_name = args[0];
            const std::string &group = args[1];
            const std::string &col_name = args[2];
            if (tg->column_map.find(col_name) == tg->column_map.end()) {
                throw std::runtime_error("Column not found: " + col_name);
            }

            db.CreateTigerIndex(table_name, group, col_name);
            res.msg = "Tiger-Index created on " + table_name + "." + group + "."+col_name;
        }

        else if (cmd == "UPDATE_SK_INDEX") {
            if (args.size() < 3) throw std::runtime_error("Req: table, group, col");
            BigTable* t = db.GetTable(args[0]);
            ColumnarTiger* tg = t->GetGroup(args[1]);
            tg->UpdateSecondaryIndex(args[2]);
            res.msg = "Secondary-Index update on " + args[2];
        }
        else if (cmd == "UPDATE_TG_INDEX") {
            if (args.size() < 3) throw std::runtime_error("Req: table, group, col");
            BigTable* t = db.GetTable(args[0]);
            ColumnarTiger* tg = t->GetGroup(args[1]);
            const std::string &table_name = args[0];
            const std::string &group = args[1];
            const std::string &col_name = args[2];
            db.UpdateTigerIndex(table_name, group, col_name);
            res.msg = "Tiger-Index update on " + col_name;
        }
        else if (cmd == "CLEAR_SK_INDEX") {
            if (args.size() < 3) throw std::runtime_error("Req: table, group, col");
            BigTable* t = db.GetTable(args[0]);
            ColumnarTiger* tg = t->GetGroup(args[1]);
            tg->ClearSecondaryIndex(args[2]);
            res.msg = "Secondary-Index update on " + args[2];
        }
        else if (cmd == "SELECT_FROM_ROWS") {
             // SELECT_FROM_ROWS(table, group, col, [id1,id2,...])
             if (args.size() < 4) throw std::runtime_error("Req: table, group, col, rows");
             BigTable* t = db.GetTable(args[0]);
             ColumnarTiger* tg = t->GetGroup(args[1]);
             
             // Parse list "[1, 2, 3]"
             std::string row_str = args[3];
             if (row_str.front() == '[') row_str = row_str.substr(1);
             if (row_str.back() == ']') row_str.pop_back();
             std::stringstream ss(row_str);
             std::string item;
             std::vector<RowID> rows;
             while(std::getline(ss, item, ',')) {
                 item = trim(item);
                 if(!item.empty()) rows.push_back(std::stoi(item));
             }
             res.msg = tg->SelectFromRows(args[2], rows);
        }
        else if (cmd == "JOIN") {
            // JOIN(table, g_a, c_a, g_b, c_b, op, val)
            if (args.size() < 7) throw std::runtime_error("Req: table, g_a, c_a, g_b, c_b, op, val");
            // std::vector<RowID> rows = db.JoinFilter(args[0], args[1], args[2], args[3], args[4], args[5], args[6]);
            // res.rows = rows;
            // res.has_rows = true;
            // res.msg = "Join Result: " + std::to_string(rows.size()) + " rows.";
            std::vector<RowID> rows = db.JoinFilter(args[0], args[1], args[2], args[3], args[4], args[5], args[6]);
            std::stringstream ss;
            for (RowID rowid : rows) {
                ss << rowid << "\n";
            }
            res.msg = ss.str();
        }
        else if (cmd == "SELECT") {
            // Existing SELECT logic...
            if (args.size() < 5) throw std::runtime_error("Req: table, group, col, limit, offset");
            BigTable* t = db.GetTable(args[0]);
            ColumnarTiger* tg = t->GetGroup(args[1]);
            std::string col = args[2];
            int limit = std::stoi(args[3]);
            int offset = std::stoi(args[4]);
            
            ColType type = tg->GetColumnType(col);
            uint32_t total = tg->GetRowCount(col);
            if ((uint32_t)offset >= total) { res.msg = "[]"; return res; }
            int end = std::min((int)total, offset + limit);
            
            std::stringstream ss;
            for (int i = offset; i < end; ++i) {
                if (type == ColType::TEXT) {
                    const std::string output_buf = tg->GetText(col, i);
                    if (output_buf.size() > MAX_OUTPUT_SIZE) {
                        ss << output_buf.substr(0, MAX_OUTPUT_SIZE)  << "<...>\n";
                    } else {
                        ss << output_buf  << "\n";
                    }
                } else if (type == ColType::INT32) {
                    ss << tg->GetInt(col, i) << "\n";
                }
                else if (type == ColType::FLOAT32) {
                    ss << tg->GetFloat(col, i) << "\n";
                }
                else if (type == ColType::VECTOR_F32) {
                    ss << "<vec>" << "\n";
                }
            }

            res.msg = ss.str();
        }
        // ... (FILTER, SEARCH, etc from previous) ...
        else if (cmd == "FILTER") {
            if (args.size() < 5) throw std::runtime_error("Req: table, group, col, op, val");
            BigTable* t = db.GetTable(args[0]);
            ColumnarTiger* tg = t->GetGroup(args[1]);
            int limit = (args.size() > 5) ? std::stoi(args[5]) : 10;
            // res.rows = tg->Filter(args[2], args[3], args[4], limit);
            // res.has_rows = res.rows.size()>0;
            // res.context_engine = tg;
            // res.msg = "Found " + std::to_string(res.rows.size()) + " matches.";
            std::vector<RowID> rows = tg->Filter(args[2], args[3], args[4], limit);
            std::stringstream ss;
            ss << "[";
            for (RowID rowid : rows) {
                ss << rowid << ",";
            }
            ss << "]";
            res.msg = ss.str();
        }
        else if (cmd == "FILTER_INDEX") {
            if (args.size() < 5) throw std::runtime_error("Req: table, group, col, op, val");
            BigTable* t = db.GetTable(args[0]);
            ColumnarTiger* tg = t->GetGroup(args[1]);
            int limit = (args.size() > 5) ? std::stoi(args[5]) : 10;
            std::vector<RowID> rows = tg->FilterIndex(args[2], args[3], args[4], limit);
            std::stringstream ss;
            ss << "[";
            for (RowID rowid : rows) {
                ss << rowid << ",";
            }
            ss << "]";
            res.msg = ss.str();
        }
        else if (cmd == "FILTER_SK_INDEX") {
            if (args.size() < 5) throw std::runtime_error("Req: table, group, col, op, val");
            BigTable* t = db.GetTable(args[0]);
            ColumnarTiger* tg = t->GetGroup(args[1]);
            int limit = (args.size() > 5) ? std::stoi(args[5]) : 10;
            std::vector<RowID> rows = tg->FilterSecondaryIndex(args[2], args[3], args[4], limit);
            std::stringstream ss;
            ss << "[";
            for (RowID rowid : rows) {
                ss << rowid << ",";
            }
            ss << "]";
            res.msg = ss.str();
            // res.rows = tg->FilterSecondaryIndex(args[2], args[3], args[4], limit);
            // res.has_rows = res.rows.size() > 0;
            // res.context_engine = tg;
            // res.msg = "Found " + std::to_string(res.rows.size()) + " matches.";
        }
        else if (cmd == "FILTER_TG_INDEX") {
            if (args.size() < 5) {
                throw std::runtime_error("Req: table, group, col, op, val");
            }

            BigTable* t = db.GetTable(args[0]);
            ColumnarTiger* tg = t->GetGroup(args[1]);

            const std::string &table_name = args[0];
            const std::string &group = args[1];
            const std::string &col_name = args[2];
            const std::string &op = args[3];
            const std::string &val = args[4];

            int limit = (args.size() > 5) ? std::stoi(args[5]) : 10;
            // res.rows = db.FilterTigerIndex(table_name, group, col_name, op, val, limit);
            // res.has_rows = res.rows.size()>0;
            // res.context_engine = tg;
            // res.msg = "Found " + std::to_string(res.rows.size()) + " matches.";
            std::vector<RowID> rows = db.FilterTigerIndex(table_name, group, col_name, op, val, limit);
            std::stringstream ss;
            ss << "[";
            for (RowID rowid : rows) {
                ss << rowid << ",";
            }
            ss << "]";
            res.msg = ss.str();
        }
        else if (cmd == "LEN") {
            if (args.size() < 4) throw std::runtime_error("Req: table, group, col, index");
            BigTable* t = db.GetTable(args[0]);
            ColumnarTiger* tg = t->GetGroup(args[1]);
            std::string col_name = args[2];

            if (tg->column_map.find(col_name) == tg->column_map.end()) {
                throw std::runtime_error("Column not found: " + col_name);
            }

            RowID rowid = std::stoi(args[3]);
            ColType type = tg->GetColumnType(col_name);

            int idx = tg->column_map.at(col_name);
            ColumnHeader& col = tg->columns[idx];

            if (rowid >= col.count) {
                throw std::runtime_error("rowid outside valid range");                
            }

            if (type==ColType::TEXT) {
                res.msg = std::to_string(tg->GetText(col_name, rowid).size());
            } else if (type==ColType::VECTOR_F32) {
                res.msg = std::to_string(col.vector_dim);
            } else {
                throw std::runtime_error("LEN IS FOR TEXT ONLY");
            }

            res.context_engine = tg;
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
        else if (cmd == "SEARCH") {
            if (args.size() < 5) throw std::runtime_error("Req: table, group, col, vec, top_k");
            BigTable* t = db.GetTable(args[0]);
            ColumnarTiger* tg = t->GetGroup(args[1]);
            auto results = tg->VectorSearch(args[2], parse_vector_data(args[3]), std::stoi(args[4]));
            std::stringstream ss;
            ss << "[";
            for(auto& r : results) {
                ss << r.id << ",";
            }
            ss << "]";
            res.msg = ss.str();
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
        else if (cmd == "STATS") { res.msg = db.GetStats(); }
        else if (cmd == "RESET") { db.Reset(); res.msg = "Reset."; }
        else { throw std::runtime_error("Unknown command: " + cmd); }
        
        return res;
    }
public:
    static std::string Execute(NativeMetaDB& db, std::string query) {
        // ... [Pipeline logic from previous impl] ...
        try {
            query = trim(query);
            if (query.empty()) return "";
            if (query.back() == ';') query.pop_back();

            size_t pipe_pos = query.find("|>");
            std::string lhs = (pipe_pos == std::string::npos) ? query : query.substr(0, pipe_pos);
            std::string rhs = (pipe_pos == std::string::npos) ? "" : query.substr(pipe_pos + 2);

            size_t paren_pos = lhs.find('(');
            if (paren_pos == std::string::npos) {
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

            if (res.has_rows) {
                 std::stringstream ss;
                 ss << "[";
                 for(size_t i=0; i<res.rows.size(); ++i) ss << res.rows[i] << (i < res.rows.size()-1 ? "," : "");
                 ss << "]";
                 return ss.str();
            }
            return res.msg;

        } catch (const std::exception& e) { return std::string("Error: ") + e.what(); }
    }
};