#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>
#include "TensorGraph.hpp"

// --- MAIN EXECUTION ---
// g++ main.cpp -o tensorgraph -O3 -march=native ./tensorgraph

int main() {
    try {
        NativeMetaDB db;
        std::cout << "=== Initializing Native AI Database Architecture ===\n";

        // ---------------------------------------------------------
        // 1. DEFINE SCHEMA: BIGTABLE 'COMPANY'
        // ---------------------------------------------------------
        BigTable* t_company = db.CreateTable("COMPANY");
        
        // Group A: Analytics/Metadata (Optimized for filtering)
        TensorGraph* cg_comp_details = t_company->CreateGroup("Details", 64, "analytics");
        cg_comp_details->CreateColumn("id", ColType::INT32, 1000);
        cg_comp_details->CreateColumn("name", ColType::TEXT, 1000);
        cg_comp_details->CreateColumn("sector", ColType::TEXT, 1000);

        // Group B: AI Embeddings (Optimized for AVX Scan)
        TensorGraph* cg_comp_vectors = t_company->CreateGroup("Embeddings", 128, "vectors");
        cg_comp_vectors->CreateColumn("semantic_vec", ColType::VECTOR_F32, 1000, 4); 

        // ---------------------------------------------------------
        // 2. DEFINE SCHEMA: BIGTABLE 'OFFICERS'
        // ---------------------------------------------------------
        BigTable* t_officers = db.CreateTable("OFFICERS");
        
        // Group A: Details & Graph Links
        TensorGraph* cg_off_details = t_officers->CreateGroup("Details", 64, "graph_nodes");
        cg_off_details->CreateColumn("name", ColType::TEXT, 1000);
        cg_off_details->CreateColumn("title", ColType::TEXT, 1000);
        // RELATIONSHIP: Point directly to Company RowID
        cg_off_details->CreateColumn("company_row_id", ColType::INT32, 1000); 

        // ---------------------------------------------------------
        // 3. INGEST DATA
        // ---------------------------------------------------------
        std::cout << "Ingesting Hierarchical Data...\n";

        // -- Company 1: "Orbital Dynamics" --
        RowID c0 = cg_comp_details->AddRow();
        cg_comp_details->SetInt("id", c0, 1001);
        cg_comp_details->SetText("name", c0, "Orbital Dynamics");
        cg_comp_details->SetText("sector", c0, "Aerospace");
        
        // Add its vector (synced row ID)
        cg_comp_vectors->AddRow();
        cg_comp_vectors->SetVector("semantic_vec", c0, {0.9, 0.8, 0.1, 0.0});

        // -- Company 2: "DeepSea Mining" --
        RowID c1 = cg_comp_details->AddRow();
        cg_comp_details->SetInt("id", c1, 1002);
        cg_comp_details->SetText("name", c1, "DeepSea Mining");
        cg_comp_details->SetText("sector", c1, "Energy");

        cg_comp_vectors->AddRow();
        cg_comp_vectors->SetVector("semantic_vec", c1, {0.1, 0.2, 0.9, 0.8});

        // -- Officers (Linked to Companies) --
        RowID off0 = cg_off_details->AddRow();
        cg_off_details->SetText("name", off0, "Commander Shepard");
        cg_off_details->SetText("title", off0, "Chief Operations");
        cg_off_details->SetInt("company_row_id", off0, c0); // Linked to Orbital (Row 0)

        RowID off1 = cg_off_details->AddRow();
        cg_off_details->SetText("name", off1, "Dr. Liara");
        cg_off_details->SetText("title", off1, "Chief Science");
        cg_off_details->SetInt("company_row_id", off1, c0); // Linked to Orbital (Row 0)

        RowID off2 = cg_off_details->AddRow();
        cg_off_details->SetText("name", off2, "Captain Nemo");
        cg_off_details->SetText("title", off2, "CEO");
        cg_off_details->SetInt("company_row_id", off2, c1); // Linked to DeepSea (Row 1)

        db.PrintStats();

        // ---------------------------------------------------------
        // 4. EXECUTE HYBRID QUERY
        // Request: "Find Officers working at Aerospace companies (Vector Similarity)"
        // ---------------------------------------------------------
        std::cout << "\n>>> Query: Find Officers at companies matching 'Aerospace' vector...\n";

        // STEP A: Vector Search (Scatter to 'Embeddings' Group)
        // Searching for a vector close to Orbital Dynamics
        std::vector<float> search_vec = {0.95, 0.85, 0.0, 0.0}; 
        auto company_matches = cg_comp_vectors->VectorSearch("semantic_vec", search_vec, 1);

        // STEP B: Graph Traversal (Gather from 'Officers' Table)
        for (const auto& match : company_matches) {
            // Materialize Parent Company Data
            std::string comp_name = cg_comp_details->GetText("name", match.id);
            std::cout << "[Match] Company: " << comp_name << " (Score: " << match.score << ")\n";

            // Traverse to Children (Officers)
            // In a real engine, we'd use a Reverse Index or Bitmap here.
            // For this demo, we scan the officers table.
            // This is FAST because it's just integer comparison in memory.
            std::cout << "   [Graph Traversal] Officers:\n";
            
            // Assuming we know we added 3 officers
            for(RowID i=0; i<3; ++i) {
                int32_t link_id = cg_off_details->GetInt("company_row_id", i);
                
                // The Join Condition
                if (link_id == match.id) {
                    std::string off_name = cg_off_details->GetText("name", i);
                    std::string off_title = cg_off_details->GetText("title", i);
                    std::cout << "    -> " << off_name << " (" << off_title << ")\n";
                }
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "Fatal DB Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}