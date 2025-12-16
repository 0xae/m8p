# TensorGraphDB: The Native AI Storage Engine

**TensorGraphDB** is a high-performance, single-header C++ database engine designed specifically for AI and RAG workloads. It bridges the gap between columnar analytics, vector search, and graph traversal without the overhead of traditional SQL databases.

---

## 🚀 Core Philosophy

1.  **Zero-Copy Architecture**: Data is stored in a single contiguous memory arena. Loading a database is as simple as memory-mapping a file. No deserialization, no parsing overhead.
2.  **Pointer Swizzling**: Relationships between entities (e.g., Author -> Posts) are stored as direct integer offsets (`RowID`). "Joins" are instant array lookups, not hash table scans.
3.  **Hybrid Storage**: Supports columnar data (`INT`, `FLOAT`, `TEXT`) alongside high-dimensional `VECTORS` (AVX-512 optimized).
4.  **No Dependencies**: Built with standard C++17. No external libraries required (except `readline` for the optional CLI shell).

---

## 🏗️ Architecture

The system is organized hierarchically to support massive scale and logical separation:

* **MetaDB**: The root container holding multiple Tables.
* **BigTable**: A logical collection of entities (e.g., "Users", "Documents"). Contains multiple *ColumnGroups*.
* **ColumnGroup**: A physical storage engine (an instance of `TensorGraph`). This allows you to store Metadata on SSDs (for analytics) and Vectors in RAM (for fast scanning) independently.
* **TensorGraph (The Engine)**: The core allocator and query processor.
    * **Arena**: A unified memory block.
    * **Columns**: Typed arrays aligned to CPU cache lines (64 bytes).

---

## 🛠️ Setup & Compilation

### Requirements
* C++17 Compiler (GCC/Clang/MSVC)
* `libreadline` (Optional, for the interactive shell)

### Build
```bash
# Standard Build
g++ main.cpp -o tensorgraph -O3 -march=native -lreadline

# With AVX-512 Support (if hardware supports it)
g++ main.cpp -o tensorgraph -O3 -march=native -mavx512f -lreadline
```

---

## 💻 CLI Commands

Launch the shell with `./tensorgraph`. Commands are case-insensitive.

### Schema Definition
```sql
-- Create a logical table
CREATE_TABLE("Users");

-- Create a physical storage group (Size in MB)
CREATE_GROUP("Users", "Profile", 64, "analytics");
CREATE_GROUP("Users", "Embeddings", 128, "vectors");

-- Define Columns
CREATE_COLUMN("Users", "Profile", "name", TEXT);
CREATE_COLUMN("Users", "Profile", "age", INT);
CREATE_COLUMN("Users", "Embeddings", "face_vec", VECTOR, 128); -- 128-dim vector
```

### Data Manipulation
```sql
-- Reserve a new row ID
ADD_ROW("Users", "Profile"); 
-- Returns: Row added. ID: 0

-- Set Data
UPDATE("Users", "Profile", "name", 0, "Alice");
UPDATE("Users", "Profile", "age", 0, 30);
UPDATE("Users", "Embeddings", "face_vec", 0, "[0.1, 0.5, ...]");
```

### Querying
```sql
-- Get specific value
GET("Users", "Profile", "name", 0, TEXT);

-- Fetch range of rows (Pagination)
-- SELECT(table, group, col, limit, offset, type_hint)
SELECT("Users", "Profile", "name", 10, 0, TEXT);

-- Vector Search (Approximate Nearest Neighbor)
-- Search "Embeddings" group, "face_vec" column, top 5 matches
SEARCH("Users", "Embeddings", "face_vec", "[0.1, 0.5, ...]", 5);
```

---

## 🔍 C++ API Example

Integrating **TensorGraphDB** into your application is simple:

```cpp
#include "TensorGraph.hpp"

int main() {
    NativeMetaDB db;
    BigTable* users = db.CreateTable("Users");
    TensorGraph* bio = users->CreateGroup("Bio", 64, "meta");
    
    bio->CreateColumn("name", ColType::TEXT, 1000);
    
    // Add Data
    RowID id = bio->AddRow();
    bio->SetText("name", id, "Admin");
    
    // Single Access
    std::cout << bio->GetText("name", id); // Output: Admin

    // Range Access (Limit 10, Offset 0)
    auto rows = bio->GetTextRange("name", 10, 0);
    for(const auto& val : rows) {
        std::cout << val << "\\n";
    }
}
```

---

**Optimized for the AI Era.** *Maximize FLOPs. Minimize Latency. Optimize CAPEX.*