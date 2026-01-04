#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include <sstream>
#include <chrono>
#include "ColumnarTiger_v1.hpp"

// --- Test Framework ---
#define ASSERT_TRUE(cond) \
    if (!(cond)) { \
        std::cerr << " [FAIL] " << __FUNCTION__ << " line " << __LINE__ << "\n"; \
        return false; \
    }

#define ASSERT_EQ_STR(val1, val2) \
    if ((val1) != (val2)) { \
        std::cerr << " [FAIL] " << __FUNCTION__ << " line " << __LINE__ << ":\n" \
                  << "   Expected: " << (val2) << "\n" \
                  << "   Actual:   " << (val1) << "\n"; \
        return false; \
    }

#define ASSERT_CONTAINS(str, sub) \
    if ((str).find(sub) == std::string::npos) { \
        std::cerr << " [FAIL] " << __FUNCTION__ << " line " << __LINE__ << ": '" \
                  << (str) << "' does not contain '" << (sub) << "'\n"; \
        return false; \
    }

// Timer for stress tests
class Timer {
    std::chrono::high_resolution_clock::time_point start;
public:
    Timer() : start(std::chrono::high_resolution_clock::now()) {}
    double elapsed_ms() {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(end - start).count();
    }
};

// --- TESTS ---

// 1. Stress Test: Indexing 10,000 Rows
bool Test_IndexStress() {
    std::cout << "   -> Generating 10,000 rows..." << std::flush;
    NativeMetaDB db;
    TGQL::Execute(db, "CREATE_TABLE(\"Stress\")");
    TGQL::Execute(db, "CREATE_GROUP(\"Stress\", \"Data\", 128, \"data\")");
    TGQL::Execute(db, "CREATE_COLUMN(\"Stress\", \"Data\", \"key\", TEXT)");
    TGQL::Execute(db, "CREATE_COLUMN(\"Stress\", \"Data\", \"val\", INT)");

    int N = 10000;
    // Batch insert simulation (loop)
    for(int i=0; i<N; ++i) {
        TGQL::Execute(db, "ADD_ROW(\"Stress\", \"Data\")");
        // Keys: "User_0", "User_1", ...
        TGQL::Execute(db, "UPDATE(\"Stress\", \"Data\", \"key\", " + std::to_string(i) + ", \"User_" + std::to_string(i) + "\")");
        TGQL::Execute(db, "UPDATE(\"Stress\", \"Data\", \"val\", " + std::to_string(i) + ", " + std::to_string(i*10) + ")");
    }
    std::cout << " Done.\n";

    // Create Index
    std::cout << "   -> Creating Index on 'key'..." << std::flush;
    Timer t;
    TGQL::Execute(db, "CREATE_INDEX(\"Stress\", \"Data\", \"key\")");
    std::cout << " Done (" << t.elapsed_ms() << "ms).\n";

    // Test Lookup Speed (Indexed Filter)
    std::cout << "   -> Performing Indexed Filter..." << std::flush;
    Timer t2;
    // Find User_5000 (Exact Match)
    std::string res = TGQL::Execute(db, "FILTER(\"Stress\", \"Data\", \"key\", \"EQ\", \"User_5000\")");
    double lookup_time = t2.elapsed_ms();
    std::cout << " Done (" << lookup_time << "ms).\n";

    ASSERT_CONTAINS(res, "Found 1 matches");
    ASSERT_CONTAINS(res, "[5000]");
    
    // Verify Value
    std::string val = TGQL::Execute(db, "GET(\"Stress\", \"Data\", \"val\", 5000, INT)");
    ASSERT_EQ_STR(val, "50000");

    return true;
}

// 2. Select From Rows (Scatter-Gather Simulation)
bool Test_SelectFromRows() {
    NativeMetaDB db;
    TGQL::Execute(db, "CREATE_TABLE(\"T\")");
    TGQL::Execute(db, "CREATE_GROUP(\"T\", \"G\", 64, \"d\")");
    TGQL::Execute(db, "CREATE_COLUMN(\"T\", \"G\", \"name\", TEXT)");
    
    // Add A, B, C, D
    TGQL::Execute(db, "ADD_ROW(\"T\", \"G\")"); TGQL::Execute(db, "UPDATE(\"T\", \"G\", \"name\", 0, \"A\")");
    TGQL::Execute(db, "ADD_ROW(\"T\", \"G\")"); TGQL::Execute(db, "UPDATE(\"T\", \"G\", \"name\", 1, \"B\")");
    TGQL::Execute(db, "ADD_ROW(\"T\", \"G\")"); TGQL::Execute(db, "UPDATE(\"T\", \"G\", \"name\", 2, \"C\")");
    TGQL::Execute(db, "ADD_ROW(\"T\", \"G\")"); TGQL::Execute(db, "UPDATE(\"T\", \"G\", \"name\", 3, \"D\")");

    // Request rows 3, 0, 2 (Out of order)
    std::string res = TGQL::Execute(db, "SELECT_FROM_ROWS(\"T\", \"G\", \"name\", \"[3, 0, 2]\")");
    
    // Expect: ["D", "A", "C"]
    ASSERT_CONTAINS(res, "\"D\", \"A\", \"C\"");
    
    return true;
}

// 3. Complex Join Scenario (1:Many Relationship)
bool Test_JoinScenario() {
    NativeMetaDB db;
    
    // --- Setup Departments ---
    TGQL::Execute(db, "CREATE_TABLE(\"Org\")");
    TGQL::Execute(db, "CREATE_GROUP(\"Org\", \"Depts\", 64, \"meta\")");
    TGQL::Execute(db, "CREATE_COLUMN(\"Org\", \"Depts\", \"dept_name\", TEXT)");
    
    TGQL::Execute(db, "ADD_ROW(\"Org\", \"Depts\")"); // 0
    TGQL::Execute(db, "UPDATE(\"Org\", \"Depts\", \"dept_name\", 0, \"Engineering\")");
    
    TGQL::Execute(db, "ADD_ROW(\"Org\", \"Depts\")"); // 1
    TGQL::Execute(db, "UPDATE(\"Org\", \"Depts\", \"dept_name\", 1, \"Sales\")");

    // Index Dept Name for fast joins
    TGQL::Execute(db, "CREATE_INDEX(\"Org\", \"Depts\", \"dept_name\")");

    // --- Setup Employees ---
    TGQL::Execute(db, "CREATE_GROUP(\"Org\", \"Emps\", 64, \"data\")");
    TGQL::Execute(db, "CREATE_COLUMN(\"Org\", \"Emps\", \"emp_name\", TEXT)");
    TGQL::Execute(db, "CREATE_COLUMN(\"Org\", \"Emps\", \"dept_link\", TEXT)"); // Text link for now

    // Add Engineers
    for(int i=0; i<5; ++i) {
        TGQL::Execute(db, "ADD_ROW(\"Org\", \"Emps\")");
        std::string id = std::to_string(i);
        TGQL::Execute(db, "UPDATE(\"Org\", \"Emps\", \"emp_name\", " + id + ", \"Eng_" + id + "\")");
        TGQL::Execute(db, "UPDATE(\"Org\", \"Emps\", \"dept_link\", " + id + ", \"Engineering\")");
    }

    // Add Salespeople
    for(int i=5; i<8; ++i) {
        TGQL::Execute(db, "ADD_ROW(\"Org\", \"Emps\")");
        std::string id = std::to_string(i);
        TGQL::Execute(db, "UPDATE(\"Org\", \"Emps\", \"emp_name\", " + id + ", \"Sales_" + id + "\")");
        TGQL::Execute(db, "UPDATE(\"Org\", \"Emps\", \"dept_link\", " + id + ", \"Sales\")");
    }

    // --- JOIN QUERY ---
    // "Find Employees working in Engineering"
    // JOIN(table, g_a (Emps), c_a (dept_link), g_b (Depts), c_b (dept_name), op (EQ), val ("Engineering"))
    // Logic:
    // 1. Filter Depts where dept_name = "Engineering" -> Gets Row 0.
    // 2. Value of Row 0 in 'dept_name' is "Engineering".
    // 3. Search Emps where dept_link = "Engineering".
    
    // Note on TGQL Join syntax implemented:
    // JOIN(table, g_a, c_a, g_b, c_b, op, val)
    // Filters B by (c_b op val). Uses resulting c_b values to match A.c_a.
    
    std::cout << "   -> Executing JOIN..." << std::flush;
    std::string res = TGQL::Execute(db, "JOIN(\"Org\", \"Emps\", \"dept_link\", \"Depts\", \"dept_name\", \"EQ\", \"Engineering\")");
    std::cout << " Done.\n";

    // Should find 5 engineers
    ASSERT_CONTAINS(res, "Join Result: 5 rows");

    // Verify content of result (using SELECT_FROM_ROWS on the IDs conceptually, 
    // but here we just check the count returned by JOIN string)
    
    return true;
}

int main() {
    std::cout << "Running ColumnarTiger Stress Tests...\n";
    int passed = 0;
    int total = 0;

    auto run = [&](bool(*f)(), const char* name) {
        total++;
        std::cout << "--------------------------------------------------\n";
        std::cout << "RUNNING: " << name << "\n";
        if(f()) {
            std::cout << " [PASS] " << name << "\n";
            passed++;
        } else {
            std::cout << " [FAIL] " << name << "\n";
        }
    };

    run(Test_IndexStress, "Test_IndexStress");
    run(Test_SelectFromRows, "Test_SelectFromRows");
    run(Test_JoinScenario, "Test_JoinScenario");

    std::cout << "\n--------------------------------------------------\n";
    std::cout << "STRESS RESULTS: " << passed << "/" << total << " passed.\n";
    return (passed == total) ? 0 : 1;
}