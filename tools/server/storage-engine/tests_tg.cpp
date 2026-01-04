#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include <sstream>
#include <fstream>
#include "ColumnarTiger_v1.hpp"

// Simple Test Framework
#define ASSERT_EQ(val1, val2) \
    if ((val1) != (val2)) { \
        std::cerr << " [FAIL] " << __FUNCTION__ << " line " << __LINE__ << ": " \
                  << (val1) << " != " << (val2) << "\n"; \
        return false; \
    }

#define ASSERT_CONTAINS(str, sub) \
    if ((str).find(sub) == std::string::npos) { \
        std::cerr << " [FAIL] " << __FUNCTION__ << " line " << __LINE__ << ": '" \
                  << (str) << "' does not contain '" << (sub) << "'\n"; \
        return false; \
    }

#define RUN_TEST(func) \
    if (func()) std::cout << " [PASS] " << #func << "\n"; \
    else std::cout << " [FAIL] " << #func << "\n";

// --- TESTS ---

// 1. Basic Table Creation
bool Test_CreateTable() {
    NativeMetaDB db;
    std::string res = TGQL::Execute(db, "CREATE_TABLE(\"Users\")");
    ASSERT_CONTAINS(res, "Table 'Users' created");
    return true;
}

// 2. Duplicate Table Error
bool Test_DuplicateTable() {
    NativeMetaDB db;
    TGQL::Execute(db, "CREATE_TABLE(\"Users\")");
    std::string res = TGQL::Execute(db, "CREATE_TABLE(\"Users\")");
    ASSERT_CONTAINS(res, "Error: Table exists");
    return true;
}

// 3. Create Column Group
bool Test_CreateGroup() {
    NativeMetaDB db;
    TGQL::Execute(db, "CREATE_TABLE(\"Users\")");
    std::string res = TGQL::Execute(db, "CREATE_GROUP(\"Users\", \"Meta\", 64, \"data\")");
    ASSERT_CONTAINS(res, "Group 'Meta' created");
    return true;
}

// 4. Create Columns (Various Types)
bool Test_CreateColumns() {
    NativeMetaDB db;
    TGQL::Execute(db, "CREATE_TABLE(\"T\")");
    TGQL::Execute(db, "CREATE_GROUP(\"T\", \"G\", 64, \"d\")");
    
    std::string r1 = TGQL::Execute(db, "CREATE_COLUMN(\"T\", \"G\", \"age\", INT)");
    ASSERT_CONTAINS(r1, "Column 'age' created");
    
    std::string r2 = TGQL::Execute(db, "CREATE_COLUMN(\"T\", \"G\", \"score\", FLOAT)");
    ASSERT_CONTAINS(r2, "Column 'score' created");
    
    std::string r3 = TGQL::Execute(db, "CREATE_COLUMN(\"T\", \"G\", \"name\", TEXT)");
    ASSERT_CONTAINS(r3, "Column 'name' created");

    return true;
}

// 5. Add Row (Capacity Check)
bool Test_AddRow() {
    NativeMetaDB db;
    TGQL::Execute(db, "CREATE_TABLE(\"T\")");
    TGQL::Execute(db, "CREATE_GROUP(\"T\", \"G\", 64, \"d\")");
    TGQL::Execute(db, "CREATE_COLUMN(\"T\", \"G\", \"c\", INT)");
    
    std::string res = TGQL::Execute(db, "ADD_ROW(\"T\", \"G\")");
    ASSERT_CONTAINS(res, "Row added. ID: 0");
    
    res = TGQL::Execute(db, "ADD_ROW(\"T\", \"G\")");
    ASSERT_CONTAINS(res, "Row added. ID: 1");
    return true;
}

// 6. Set/Get Integer
bool Test_IntOps() {
    NativeMetaDB db;
    TGQL::Execute(db, "CREATE_TABLE(\"T\")");
    TGQL::Execute(db, "CREATE_GROUP(\"T\", \"G\", 64, \"d\")");
    TGQL::Execute(db, "CREATE_COLUMN(\"T\", \"G\", \"val\", INT)");
    TGQL::Execute(db, "ADD_ROW(\"T\", \"G\")");
    
    TGQL::Execute(db, "UPDATE(\"T\", \"G\", \"val\", 0, 42)");
    std::string get = TGQL::Execute(db, "GET(\"T\", \"G\", \"val\", 0, INT)");
    ASSERT_EQ(get, "42");
    return true;
}

// 7. Set/Get Float
bool Test_FloatOps() {
    NativeMetaDB db;
    TGQL::Execute(db, "CREATE_TABLE(\"T\")");
    TGQL::Execute(db, "CREATE_GROUP(\"T\", \"G\", 64, \"d\")");
    TGQL::Execute(db, "CREATE_COLUMN(\"T\", \"G\", \"val\", FLOAT)");
    TGQL::Execute(db, "ADD_ROW(\"T\", \"G\")");
    
    TGQL::Execute(db, "UPDATE(\"T\", \"G\", \"val\", 0, 3.14)");
    std::string get = TGQL::Execute(db, "GET(\"T\", \"G\", \"val\", 0, FLOAT)");
    // String conversion might vary slightly, check prefix
    ASSERT_CONTAINS(get, "3.14"); 
    return true;
}

// 8. Set/Get Text
bool Test_TextOps() {
    NativeMetaDB db;
    TGQL::Execute(db, "CREATE_TABLE(\"T\")");
    TGQL::Execute(db, "CREATE_GROUP(\"T\", \"G\", 64, \"d\")");
    TGQL::Execute(db, "CREATE_COLUMN(\"T\", \"G\", \"name\", TEXT)");
    TGQL::Execute(db, "ADD_ROW(\"T\", \"G\")");
    
    TGQL::Execute(db, "UPDATE(\"T\", \"G\", \"name\", 0, \"Alice\")");
    std::string get = TGQL::Execute(db, "GET(\"T\", \"G\", \"name\", 0, TEXT)");
    ASSERT_EQ(get, "Alice");
    return true;
}

// 9. Select Range (Pagination)
bool Test_Select() {
    NativeMetaDB db;
    TGQL::Execute(db, "CREATE_TABLE(\"T\")");
    TGQL::Execute(db, "CREATE_GROUP(\"T\", \"G\", 64, \"d\")");
    TGQL::Execute(db, "CREATE_COLUMN(\"T\", \"G\", \"id\", INT)");
    
    for(int i=0; i<5; ++i) {
        TGQL::Execute(db, "ADD_ROW(\"T\", \"G\")");
        TGQL::Execute(db, "UPDATE(\"T\", \"G\", \"id\", " + std::to_string(i) + ", " + std::to_string(i*10) + ")");
    }

    std::string res = TGQL::Execute(db, "SELECT(\"T\", \"G\", \"id\", 2, 1, INT)");
    // Should verify rows 1 and 2 (values 10 and 20)
    ASSERT_CONTAINS(res, "[1] 10");
    ASSERT_CONTAINS(res, "[2] 20");
    return true;
}

// 10. Select Out of Bounds
bool Test_SelectOOB() {
    NativeMetaDB db;
    TGQL::Execute(db, "CREATE_TABLE(\"T\")");
    TGQL::Execute(db, "CREATE_GROUP(\"T\", \"G\", 64, \"d\")");
    TGQL::Execute(db, "CREATE_COLUMN(\"T\", \"G\", \"id\", INT)");
    TGQL::Execute(db, "ADD_ROW(\"T\", \"G\")");

    std::string res = TGQL::Execute(db, "SELECT(\"T\", \"G\", \"id\", 10, 5, INT)");
    ASSERT_CONTAINS(res, "Offset out of bounds");
    return true;
}

// 11. Create Vector Column
bool Test_CreateVector() {
    NativeMetaDB db;
    TGQL::Execute(db, "CREATE_TABLE(\"T\")");
    TGQL::Execute(db, "CREATE_GROUP(\"T\", \"G\", 64, \"d\")");
    std::string res = TGQL::Execute(db, "CREATE_COLUMN(\"T\", \"G\", \"emb\", VECTOR, 4)");
    ASSERT_CONTAINS(res, "Column 'emb' created");
    return true;
}

// 12. Set Vector Data
bool Test_SetVector() {
    NativeMetaDB db;
    TGQL::Execute(db, "CREATE_TABLE(\"T\")");
    TGQL::Execute(db, "CREATE_GROUP(\"T\", \"G\", 64, \"d\")");
    TGQL::Execute(db, "CREATE_COLUMN(\"T\", \"G\", \"emb\", VECTOR, 2)");
    TGQL::Execute(db, "ADD_ROW(\"T\", \"G\")");
    
    std::string res = TGQL::Execute(db, "UPDATE(\"T\", \"G\", \"emb\", 0, \"[0.5, 0.9]\")");
    ASSERT_CONTAINS(res, "Updated");
    return true;
}

// 13. Get Vector Data via Select
bool Test_GetVector() {
    NativeMetaDB db;
    TGQL::Execute(db, "CREATE_TABLE(\"T\")");
    TGQL::Execute(db, "CREATE_GROUP(\"T\", \"G\", 64, \"d\")");
    TGQL::Execute(db, "CREATE_COLUMN(\"T\", \"G\", \"emb\", VECTOR, 2)");
    TGQL::Execute(db, "ADD_ROW(\"T\", \"G\")");
    TGQL::Execute(db, "UPDATE(\"T\", \"G\", \"emb\", 0, \"[0.1, 0.2]\")");
    
    std::string res = TGQL::Execute(db, "SELECT(\"T\", \"G\", \"emb\", 1, 0, VECTOR)");
    ASSERT_CONTAINS(res, "[0] [0.1, 0.2]");
    return true;
}

// 14. Vector Search (Exact Match)
bool Test_VectorSearch() {
    NativeMetaDB db;
    TGQL::Execute(db, "CREATE_TABLE(\"T\")");
    TGQL::Execute(db, "CREATE_GROUP(\"T\", \"G\", 64, \"d\")");
    TGQL::Execute(db, "CREATE_COLUMN(\"T\", \"G\", \"emb\", VECTOR, 4)");
    
    TGQL::Execute(db, "ADD_ROW(\"T\", \"G\")");
    TGQL::Execute(db, "UPDATE(\"T\", \"G\", \"emb\", 0, \"[1.0, 0.0, 0.0, 0.0]\")");
    
    TGQL::Execute(db, "ADD_ROW(\"T\", \"G\")");
    TGQL::Execute(db, "UPDATE(\"T\", \"G\", \"emb\", 1, \"[0.0, 1.0, 0.0, 0.0]\")"); // Different

    // Search for 1st vector
    std::string res = TGQL::Execute(db, "SEARCH(\"T\", \"G\", \"emb\", \"[1.0, 0.0, 0.0, 0.0]\", 1)");
    
    // Result should be ID 0 with score 0 (perfect match)
    ASSERT_CONTAINS(res, "ID: 0 Score: 0");
    return true;
}

// 15. Vector Search (Nearest Neighbor)
bool Test_VectorNearest() {
    NativeMetaDB db;
    TGQL::Execute(db, "CREATE_TABLE(\"T\")");
    TGQL::Execute(db, "CREATE_GROUP(\"T\", \"G\", 64, \"d\")");
    TGQL::Execute(db, "CREATE_COLUMN(\"T\", \"G\", \"emb\", VECTOR, 2)");
    
    TGQL::Execute(db, "ADD_ROW(\"T\", \"G\")"); // ID 0
    TGQL::Execute(db, "UPDATE(\"T\", \"G\", \"emb\", 0, \"[0.0, 0.0]\")");
    
    TGQL::Execute(db, "ADD_ROW(\"T\", \"G\")"); // ID 1
    TGQL::Execute(db, "UPDATE(\"T\", \"G\", \"emb\", 1, \"[10.0, 10.0]\")"); 

    // Search for [0.1, 0.1] -> Should match ID 0
    std::string res = TGQL::Execute(db, "SEARCH(\"T\", \"G\", \"emb\", \"[0.1, 0.1]\", 1)");
    
    ASSERT_CONTAINS(res, "ID: 0");
    return true;
}

// 16. Invalid Column Type Error
bool Test_InvalidType() {
    NativeMetaDB db;
    std::string res = TGQL::Execute(db, "CREATE_COLUMN(\"T\", \"G\", \"c\", UNKNOWN)");
    ASSERT_CONTAINS(res, "Error:");
    return true;
}

// 17. Invalid Table Access
bool Test_InvalidTable() {
    NativeMetaDB db;
    std::string res = TGQL::Execute(db, "CREATE_GROUP(\"Missing\", \"G\", 64, \"d\")");
    ASSERT_CONTAINS(res, "Error: Table not found");
    return true;
}

// 18. Invalid Group Access
bool Test_InvalidGroup() {
    NativeMetaDB db;
    TGQL::Execute(db, "CREATE_TABLE(\"T\")");
    std::string res = TGQL::Execute(db, "ADD_ROW(\"T\", \"Missing\")");
    ASSERT_CONTAINS(res, "Error: Group not found");
    return true;
}

// 19. Vector Dimension Mismatch
bool Test_VectorDimMismatch() {
    NativeMetaDB db;
    TGQL::Execute(db, "CREATE_TABLE(\"T\")");
    TGQL::Execute(db, "CREATE_GROUP(\"T\", \"G\", 64, \"d\")");
    TGQL::Execute(db, "CREATE_COLUMN(\"T\", \"G\", \"emb\", VECTOR, 3)");
    TGQL::Execute(db, "ADD_ROW(\"T\", \"G\")");
    
    // Try to set 2D vector to 3D column
    std::string res = TGQL::Execute(db, "UPDATE(\"T\", \"G\", \"emb\", 0, \"[1.0, 2.0]\")");
    ASSERT_CONTAINS(res, "Error: Type/Dim mismatch");
    return true;
}

// 20. Empty Query
bool Test_EmptyQuery() {
    NativeMetaDB db;
    std::string res = TGQL::Execute(db, "");
    ASSERT_EQ(res, "");
    return true;
}

// 21. Persistence Test
bool Test_Persistence() {
    {
        NativeMetaDB db;
        TGQL::Execute(db, "CREATE_TABLE(\"P\")");
        TGQL::Execute(db, "CREATE_GROUP(\"P\", \"G\", 64, \"d\")");
        TGQL::Execute(db, "CREATE_COLUMN(\"P\", \"G\", \"val\", INT)");
        TGQL::Execute(db, "ADD_ROW(\"P\", \"G\")");
        TGQL::Execute(db, "UPDATE(\"P\", \"G\", \"val\", 0, 99)");
        TGQL::Execute(db, "SAVE(\"./testdb\")");
    }
    
    // Reload in new instance
    {
        NativeMetaDB db;
        std::string res = TGQL::Execute(db, "LOAD(\"./testdb\")");
        ASSERT_CONTAINS(res, "loaded from prefix");
        
        std::string val = TGQL::Execute(db, "GET(\"P\", \"G\", \"val\", 0, INT)");
        ASSERT_EQ(val, "99");
    }
    return true;
}

int main() {
    std::cout << "Running TensorGraphDB Test Suite...\n";
    int passed = 0;
    int total = 0;

    auto run = [&](bool(*f)(), const char* name) {
        total++;
        if(f()) {
            std::cout << " [PASS] " << name << "\n";
            passed++;
        } else {
            std::cout << " [FAIL] " << name << "\n";
        }
    };

    run(Test_CreateTable, "Test_CreateTable");
    run(Test_DuplicateTable, "Test_DuplicateTable");
    run(Test_CreateGroup, "Test_CreateGroup");
    run(Test_CreateColumns, "Test_CreateColumns");
    run(Test_AddRow, "Test_AddRow");
    run(Test_IntOps, "Test_IntOps");
    run(Test_FloatOps, "Test_FloatOps");
    run(Test_TextOps, "Test_TextOps");
    run(Test_Select, "Test_Select");
    run(Test_SelectOOB, "Test_SelectOOB");
    run(Test_CreateVector, "Test_CreateVector");
    run(Test_SetVector, "Test_SetVector");
    run(Test_GetVector, "Test_GetVector");
    run(Test_VectorSearch, "Test_VectorSearch");
    run(Test_VectorNearest, "Test_VectorNearest");
    run(Test_InvalidType, "Test_InvalidType");
    run(Test_InvalidTable, "Test_InvalidTable");
    run(Test_InvalidGroup, "Test_InvalidGroup");
    run(Test_VectorDimMismatch, "Test_VectorDimMismatch");
    run(Test_EmptyQuery, "Test_EmptyQuery");
    run(Test_Persistence, "Test_Persistence");

    std::cout << "\nResults: " << passed << "/" << total << " tests passed.\n";
    return (passed == total) ? 0 : 1;
}