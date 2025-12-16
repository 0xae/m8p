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

// --- Configuration & Constants ---
// Define alignment for AVX-512 (64 bytes) or AVX2 (32 bytes)
constexpr size_t ALIGNMENT_BYTES = 64; 
constexpr uint32_t DELETED_FLAG = 0xFFFFFFFF; // Sentinel for deleted text offset

// --- Types ---
using RelPtr = uint32_t; // Offset from base pointer
using RowID = uint32_t;

enum class ColType : uint8_t {
    INT32,
    FLOAT32,
    TEXT, // Fixed-size header pointing to variable heap
    VECTOR_F32
};

struct ColumnHeader {
    char name[32];
    ColType type;
    uint32_t count;      // Number of rows (active + deleted)
    uint32_t capacity;   // Max rows currently allocated
    uint16_t vector_dim; // Only for VECTOR_F32
    RelPtr data_offset;  // Points to the data array start
};

// --- SIMD Helpers (Placeholder for actual intrinsics) ---
inline float l2_sq_simd(const float* a, const float* b, int dim) {
    float sum = 0.0f;
    // Auto-vectorization friendly loop
    // In production, replace with _mm256_fmadd_ps etc.
    for(int i=0; i<dim; ++i) {
        float diff = a[i] - b[i];
        sum += diff * diff;
    }
    return sum;
}
inline float l2_sq_avx(const float* a, const float* b, int dim) {
    __m256 sum = _mm256_setzero_ps();
    for (int i = 0; i < dim; i += 8) {
        __m256 v1 = _mm256_loadu_ps(a + i);
        __m256 v2 = _mm256_loadu_ps(b + i);
        __m256 diff = _mm256_sub_ps(v1, v2);
        sum = _mm256_fmadd_ps(diff, diff, sum);
    }
    float result[8];
    _mm256_storeu_ps(result, sum);
    return result[0] + result[1] + result[2] + result[3] + 
           result[4] + result[5] + result[6] + result[7];
}

struct Query {
    NativeDB* db;
    
    // "Find Chunks written by User 'Alice'"
    // No Hash Join. Just array indexing.
    void traverse_filter(int chunk_col_idx, int author_col_idx, const std::string& target_name) {
        auto* chunks_author_ids = db->get_ptr<RowID>(db->columns[chunk_col_idx].data_offset);
        auto* author_names = db->get_ptr<char>(db->columns[author_col_idx].data_offset); // Simplified
        
        int count = db->columns[chunk_col_idx].count;
        
        for(int i=0; i<count; ++i) {
            RowID author_id = chunks_author_ids[i];
            // Direct memory jump to check author name. 
            // In reality, this would check a pre-computed bitmap or index.
            if (is_author_match(author_id, target_name)) {
                mark_result(i);
            }
        }
    }
};

// --- TensorGraph Engine ---
class TensorGraph {
private:
    uint8_t* memory_block;
    size_t capacity_bytes;
    size_t head; // Allocator head
    
    // Schema Registry (Stored in RAM for easy lookups, could be serialized)
    std::unordered_map<std::string, int> column_map; // Name -> Index in headers
    std::vector<ColumnHeader> columns;

    // --- Allocator ---
    inline size_t align_forward(size_t ptr, size_t align) {
        return (ptr + align - 1) & ~(align - 1);
    }

    // Allocate storage inside the arena
    RelPtr allocate(size_t size_bytes, size_t manual_align = 0) {
        size_t align = (manual_align > 0) ? manual_align : ALIGNMENT_BYTES;
        size_t aligned_offset = align_forward(head, align);
        
        if (aligned_offset + size_bytes > capacity_bytes) {
            throw std::runtime_error("TensorGraph OOM: Arena capacity exceeded");
        }
        
        head = aligned_offset + size_bytes;
        return (RelPtr)aligned_offset;
    }

    // Helper to get typed pointer from offset
    template <typename T>
    T* get_ptr(RelPtr offset) {
        return reinterpret_cast<T*>(memory_block + offset);
    }

    // Helper to get typed pointer to specific row in column
    template <typename T>
    T* get_cell_ptr(int col_idx, RowID row) {
        if (col_idx < 0 || col_idx >= columns.size()) throw std::runtime_error("Invalid column index");
        ColumnHeader& col = columns[col_idx];
        if (row >= col.capacity) throw std::runtime_error("Row index out of bounds");
        
        // For simple types
        return get_ptr<T>(col.data_offset) + row;
    }

public:
    TensorGraph(size_t size_mb) {
        capacity_bytes = size_mb * 1024 * 1024;
        // Use aligned_alloc or posix_memalign in Linux
        // For simplicity here using standard new but ensuring alignment manually
        // Ideally: memory_block = (uint8_t*)std::aligned_alloc(4096, capacity_bytes);
        memory_block = new uint8_t[capacity_bytes]; 
        std::memset(memory_block, 0, capacity_bytes);
        head = 0; // Start at offset 0
        
        // Reserve space for a "Heap" area at the end? 
        // For now, simple linear allocation.
    }

    ~TensorGraph() {
        delete[] memory_block;
    }

    // --- API: Create Column ---
    void CreateColumn(const std::string& name, ColType type, uint32_t initial_capacity, int dim = 0) {
        if (column_map.find(name) != column_map.end()) {
            throw std::runtime_error("Column already exists: " + name);
        }

        ColumnHeader header;
        std::strncpy(header.name, name.c_str(), 31);
        header.name[31] = '\0';
        header.type = type;
        header.count = 0;
        header.capacity = initial_capacity;
        header.vector_dim = (type == ColType::VECTOR_F32) ? dim : 0;

        // Allocate Data Block
        size_t element_size = 0;
        size_t alignment = 0; // 0 = default (64)

        switch(type) {
            case ColType::INT32: element_size = sizeof(int32_t); alignment = 4; break;
            case ColType::FLOAT32: element_size = sizeof(float); alignment = 4; break;
            case ColType::TEXT: element_size = sizeof(RelPtr); alignment = 4; break; // Stores offset to heap
            case ColType::VECTOR_F32: element_size = sizeof(float) * dim; alignment = 64; break; // Align vectors for SIMD
        }

        header.data_offset = allocate(element_size * initial_capacity, alignment);
        
        columns.push_back(header);
        column_map[name] = columns.size() - 1;
    }

    // --- API: Add Row (Simplified: Adds to ALL columns) ---
    // Note: In a real DB, you'd insert dicts or tuples. 
    // Here we assume user calls specific typed setters after "reserving" a row.
    RowID AddRow() {
        // Naive: assumes all columns grow together. 
        // Real columnar stores might compress/grow independently.
        // Check capacity of first column (assuming sync)
        if (columns.empty()) return 0;
        
        uint32_t current_count = columns[0].count;
        if (current_count >= columns[0].capacity) {
            throw std::runtime_error("Column capacity reached (Resize not impl)");
        }

        for (auto& col : columns) {
            col.count++;
        }
        return current_count; 
    }

    // --- API: Setters ---
    void SetInt(const std::string& col_name, RowID row, int32_t val) {
        int idx = column_map.at(col_name);
        if(columns[idx].type != ColType::INT32) throw std::runtime_error("Type mismatch");
        *get_cell_ptr<int32_t>(idx, row) = val;
    }

    void SetFloat(const std::string& col_name, RowID row, float val) {
        int idx = column_map.at(col_name);
        if(columns[idx].type != ColType::FLOAT32) throw std::runtime_error("Type mismatch");
        *get_cell_ptr<float>(idx, row) = val;
    }

    void SetVector(const std::string& col_name, RowID row, const std::vector<float>& vec) {
        int idx = column_map.at(col_name);
        ColumnHeader& col = columns[idx];
        if(col.type != ColType::VECTOR_F32) throw std::runtime_error("Type mismatch");
        if(vec.size() != col.vector_dim) throw std::runtime_error("Dimension mismatch");

        // Calculate pointer to start of vector in flat array
        // data_offset + (row * dim * sizeof(float))
        float* dest = get_ptr<float>(col.data_offset) + (row * col.vector_dim);
        std::memcpy(dest, vec.data(), vec.size() * sizeof(float));
    }

    // Text is tricky. For simple "Arena", we append string to end of allocations and store offset.
    void SetText(const std::string& col_name, RowID row, const std::string& text) {
        int idx = column_map.at(col_name);
        if(columns[idx].type != ColType::TEXT) throw std::runtime_error("Type mismatch");
        
        // 1. Allocate string bytes (length + null terminator)
        // Note: This is append-only. Updating text leaks old memory in this naive impl.
        RelPtr str_offset = allocate(text.size() + 1, 1);
        char* str_dest = get_ptr<char>(str_offset);
        std::memcpy(str_dest, text.c_str(), text.size() + 1);

        // 2. Store offset in column
        *get_cell_ptr<RelPtr>(idx, row) = str_offset;
    }

    // --- API: Getters ---
    int32_t GetInt(const std::string& col_name, RowID row) {
        return *get_cell_ptr<int32_t>(column_map.at(col_name), row);
    }
    
    float GetFloat(const std::string& col_name, RowID row) {
        return *get_cell_ptr<float>(column_map.at(col_name), row);
    }

    std::string GetText(const std::string& col_name, RowID row) {
        RelPtr offset = *get_cell_ptr<RelPtr>(column_map.at(col_name), row);
        if (offset == DELETED_FLAG) return ""; // Deleted
        return std::string(get_ptr<char>(offset));
    }

    std::vector<float> GetVector(const std::string& col_name, RowID row) {
        int idx = column_map.at(col_name);
        ColumnHeader& col = columns[idx];
        float* src = get_ptr<float>(col.data_offset) + (row * col.vector_dim);
        return std::vector<float>(src, src + col.vector_dim);
    }

    // --- API: Delete (Soft Delete) ---
    // In columnar store, hard delete requires shifting data. Soft delete usually uses a mask.
    // Here we simulate by zeroing or flagging.
    void DeleteRow(RowID row) {
        // Mark text pointers as deleted? 
        // Real impl: Use a separate BITMAP column for validity.
        // For simplicity: We don't shift, just assume logic handles "gaps".
        std::cout << "[DB] Row " << row << " marked deleted (Logic not fully impl)\n";
    }

    // --- API: Vector Search (Brute Force Scan) ---
    struct SearchResult {
        RowID id;
        float score;
    };

    // Simple Linear Scan (Flat Index)
    std::vector<SearchResult> VectorSearch(const std::string& col_name, const std::vector<float>& query, int top_k) {
        int idx = column_map.at(col_name);
        ColumnHeader& col = columns[idx];
        
        if (query.size() != col.vector_dim) throw std::runtime_error("Query dim mismatch");

        std::vector<SearchResult> results;
        results.reserve(col.count);

        float* data_base = get_ptr<float>(col.data_offset);

        // Hot Loop
        for(uint32_t i=0; i<col.count; ++i) {
            float* target = data_base + (i * col.vector_dim);
            float dist = l2_sq_simd(query.data(), target, col.vector_dim);
            results.push_back({i, dist});
        }

        // Sort (Partial sort is faster)
        // For L2 distance, smaller is better.
        std::partial_sort(results.begin(), results.begin() + std::min((size_t)top_k, results.size()), results.end(), 
            [](const SearchResult& a, const SearchResult& b) {
                return a.score < b.score; 
            });

        if (results.size() > top_k) results.resize(top_k);
        return results;
    }

    void PrintStats() {
        std::cout << "\n=== TensorGraph Stats ===\n";
        std::cout << "Arena Used: " << head << " / " << capacity_bytes << " bytes\n";
        std::cout << "Columns: " << columns.size() << "\n";
        for(const auto& col : columns) {
            std::cout << " - " << col.name << " (Rows: " << col.count << ")\n";
        }
        std::cout << "=========================\n";
    }
};