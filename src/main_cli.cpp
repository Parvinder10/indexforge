#include "indexforge/engine_api.h"
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

void print_help() {
    std::cout << "IndexForge — High-Performance Full-Text Search Engine CLI\n"
              << "Usage: indexforge_cli <command> [options]\n\n"
              << "Commands:\n"
              << "  search     Search the indexed documents\n"
              << "             --query <q>      Search query string\n"
              << "             --algo <bm25|tfidf> Ranking algorithm (default: bm25)\n"
              << "             --k1 <val>       BM25 k1 parameter (default: 1.2)\n"
              << "             --b <val>        BM25 b parameter (default: 0.75)\n"
              << "             --limit <n>      Max results (default: 10)\n"
              << "             --linear         Run baseline linear scan comparison\n"
              << "  ingest     Ingest a document into the index\n"
              << "             --id <num>       Document ID\n"
              << "             --file <path>    Text file to read\n"
              << "             --title <str>    Document title\n"
              << "             --source <str>   Document source\n"
              << "             --type <str>     Document type\n"
              << "  save       Save index to binary file\n"
              << "             --path <file>    Output index file path\n"
              << "  load       Load index from binary file\n"
              << "             --path <file>    Input index file path\n"
              << "  verify     Verify index file integrity with CRC32\n"
              << "             --path <file>    Index file path to verify\n"
              << "  stats      Print index corpus statistics\n"
              << "  help       Show this help message\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_help();
        return 1;
    }

    std::string command = argv[1];
    if (command == "help" || command == "--help" || command == "-h") {
        print_help();
        return 0;
    }

    if (command == "verify") {
        std::string path;
        for (int i = 2; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--path" && i + 1 < argc) path = argv[++i];
        }
        if (path.empty()) {
            std::cerr << "Error: --path required for verify\n";
            return 1;
        }
        const char* res = indexforge_verify_integrity(path.c_str());
        std::cout << res << std::endl;
        indexforge_free_string(const_cast<char*>(res));
        return 0;
    }

    indexforge_handle engine = indexforge_create();
    if (!engine) {
        std::cerr << "Error: Failed to initialize IndexForge engine\n";
        return 1;
    }

    // Default sample documents for quick CLI interactive testing if starting empty
    indexforge_index_document(engine, 1, 
        "Federal Procurement Regulation Notice", 
        "General Services Administration", 
        "2026-03-15", 
        "regulation", 
        "Public tender contracts for information technology modernization require compliance with federal security guidelines and competitive bidding processes.");

    indexforge_index_document(engine, 2, 
        "Public Tender for Cloud Infrastructure", 
        "Department of Transportation", 
        "2026-05-20", 
        "tender", 
        "Solicitation notice for enterprise cloud infrastructure hosting, high availability data centers, and multi-region failover architecture.");

    indexforge_index_document(engine, 3, 
        "Cybersecurity Policy and Incident Reporting Guidelines", 
        "Cybersecurity and Infrastructure Security Agency", 
        "2026-08-10", 
        "policy", 
        "All contractors and public agencies must maintain compliance with mandatory vulnerability disclosure and rapid security incident reporting.");

    if (command == "search") {
        std::string query = "public tender";
        int algo = 0; // BM25
        double k1 = 1.2, b = 0.75;
        uint32_t limit = 10;
        bool linear = false;

        for (int i = 2; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--query" && i + 1 < argc) query = argv[++i];
            else if (arg == "--algo" && i + 1 < argc) {
                std::string a = argv[++i];
                algo = (a == "tfidf") ? 1 : 0;
            }
            else if (arg == "--k1" && i + 1 < argc) k1 = std::stod(argv[++i]);
            else if (arg == "--b" && i + 1 < argc) b = std::stod(argv[++i]);
            else if (arg == "--limit" && i + 1 < argc) limit = std::stoul(argv[++i]);
            else if (arg == "--linear") linear = true;
        }

        if (linear) {
            std::cout << "Executing Linear Document Scan Baseline...\n";
            const char* res = indexforge_linear_search(engine, query.c_str(), limit);
            std::cout << res << std::endl;
            indexforge_free_string(const_cast<char*>(res));
        } else {
            std::cout << "Executing Inverted Index Search (" << (algo == 0 ? "BM25" : "TF-IDF") << ")...\n";
            const char* res = indexforge_search(engine, query.c_str(), algo, k1, b, limit, 0);
            std::cout << res << std::endl;
            indexforge_free_string(const_cast<char*>(res));
        }
    } else if (command == "stats") {
        const char* res = indexforge_get_stats(engine);
        std::cout << res << std::endl;
        indexforge_free_string(const_cast<char*>(res));
    } else if (command == "save") {
        std::string path = "index.forge";
        for (int i = 2; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--path" && i + 1 < argc) path = argv[++i];
        }
        int rc = indexforge_save_index(engine, path.c_str());
        std::cout << (rc == 0 ? "Index successfully saved to: " : "Error saving to: ") << path << std::endl;
    } else if (command == "load") {
        std::string path = "index.forge";
        for (int i = 2; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--path" && i + 1 < argc) path = argv[++i];
        }
        int rc = indexforge_load_index(engine, path.c_str());
        std::cout << (rc == 0 ? "Index successfully loaded from: " : "Error loading from: ") << path << std::endl;
    } else {
        std::cerr << "Unknown command: " << command << "\n";
        print_help();
    }

    indexforge_destroy(engine);
    return 0;
}
