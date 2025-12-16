#include <iostream>
#include "TensorGraph.hpp"

int main() {
    try {
        // 1. Initialize DB with 64MB Arena
        TensorGraph db(64); 

        // 2. Define Schema
        std::cout << "Creating Columns...\n";
        db.CreateColumn("id", ColType::INT32, 1000);
        db.CreateColumn("content", ColType::TEXT, 1000);
        db.CreateColumn("embedding", ColType::VECTOR_F32, 1000, 4); // 4-dim vector

        // 3. Add Data
        std::cout << "Inserting Data...\n";
        
        // Row 0
        RowID r0 = db.AddRow();
        db.SetInt("id", r0, 101);
        db.SetText("content", r0, "Apple is a fruit");
        db.SetVector("embedding", r0, {1.0, 0.0, 0.0, 0.0});

        // Row 1
        RowID r1 = db.AddRow();
        db.SetInt("id", r1, 102);
        db.SetText("content", r1, "Sky is blue");
        db.SetVector("embedding", r1, {0.0, 1.0, 0.0, 0.0});

        // Row 2
        RowID r2 = db.AddRow();
        db.SetInt("id", r2, 103);
        db.SetText("content", r2, "Banana is yellow");
        db.SetVector("embedding", r2, {0.9, 0.1, 0.0, 0.0}); // Close to Apple

        db.PrintStats();

        // 4. Search
        std::cout << "\nSearching for vector close to {1.0, 0.0...} (Apple)...\n";
        std::vector<float> query = {1.0, 0.0, 0.0, 0.0};
        
        auto results = db.VectorSearch("embedding", query, 2);

        for (const auto& res : results) {
            std::string text = db.GetText("content", res.id);
            int id = db.GetInt("id", res.id);
            std::cout << "Found Row " << res.id 
                      << " (ID: " << id << ")"
                      << " Score: " << res.score 
                      << " Text: " << text << "\n";
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}