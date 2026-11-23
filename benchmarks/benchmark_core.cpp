#include "indexforge/inverted_index.hpp"
#include "indexforge/retrieval_engine.hpp"
#include "indexforge/query_parser.hpp"
#include "indexforge/ranking.hpp"
#include "indexforge/linear_scanner.hpp"
#include "indexforge/persistence.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <algorithm>
#include <random>
#include <thread>
#include <atomic>
#include <iomanip>
#include <sstream>

using namespace indexforge;

// Vocabulary pool of realistic public sector / tender / policy terms
static const std::vector<std::string> VOCAB = {
    "procurement", "regulation", "compliance", "tender", "contract", "federal",
    "infrastructure", "cybersecurity", "disclosure", "transportation", "energy",
    "modernization", "architecture", "encryption", "bidding", "audit", "policy",
    "standard", "framework", "guidelines", "vendor", "cloud", "security", "data",
    "privacy", "governance", "public", "notice", "grant", "requirement", "solicitation"
};

// Generate realistic synthetic documents
std::vector<Document> generate_corpus(size_t doc_count, unsigned int seed = 42) {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<size_t> vocab_dist(0, VOCAB.size() - 1);
    std::uniform_int_distribution<size_t> len_dist(40, 150);
    std::vector<std::string> doc_types = {"notice", "tender", "policy", "regulation", "tech_doc"};
    std::vector<std::string> sources = {"GSA", "DoD", "NIST", "CISA", "DOT", "EPA", "OMB"};

    std::vector<Document> corpus;
    corpus.reserve(doc_count);

    for (size_t i = 1; i <= doc_count; ++i) {
        size_t len = len_dist(rng);
        std::string content;
        content.reserve(len * 10);
        for (size_t w = 0; w < len; ++w) {
            if (w > 0) content.push_back(' ');
            content += VOCAB[vocab_dist(rng)];
        }

        corpus.push_back(Document{
            .id = static_cast<DocId>(i),
            .title = "Public Document #" + std::to_string(i),
            .source = sources[i % sources.size()],
            .date = "2026-0" + std::to_string((i % 9) + 1) + "-15",
            .doc_type = doc_types[i % doc_types.size()],
            .content = std::move(content)
        });
    }
    return corpus;
}

struct LatencyStats {
    double min_ms{0.0};
    double p50_ms{0.0};
    double p95_ms{0.0};
    double p99_ms{0.0};
    double max_ms{0.0};
    double mean_ms{0.0};
    double qps{0.0};
};

LatencyStats compute_latency_stats(std::vector<double>& latencies) {
    if (latencies.empty()) return {};
    std::sort(latencies.begin(), latencies.end());
    size_t n = latencies.size();

    double sum = 0.0;
    for (double l : latencies) sum += l;

    return LatencyStats{
        .min_ms = latencies.front(),
        .p50_ms = latencies[n * 50 / 100],
        .p95_ms = latencies[n * 95 / 100],
        .p99_ms = latencies[n * 99 / 100],
        .max_ms = latencies.back(),
        .mean_ms = sum / n,
        .qps = n / (sum / 1000.0)
    };
}

void run_benchmark_suite(size_t corpus_size) {
    std::cout << "\n===============================================================\n";
    std::cout << "BENCHMARK EXPERIMENT: Corpus Size = " << corpus_size << " documents\n";
    std::cout << "===============================================================\n";

    // 1. Generation
    auto gen_start = std::chrono::high_resolution_clock::now();
    auto corpus = generate_corpus(corpus_size);
    auto gen_end = std::chrono::high_resolution_clock::now();
    double gen_time = std::chrono::duration<double, std::milli>(gen_end - gen_start).count();
    std::cout << "[Step 1] Generated " << corpus_size << " realistic documents in " << gen_time << " ms\n";

    // 2. Inverted Index Construction
    InvertedIndex index;
    auto idx_start = std::chrono::high_resolution_clock::now();
    for (const auto& doc : corpus) {
        index.add_document(doc);
    }
    auto idx_end = std::chrono::high_resolution_clock::now();
    double idx_time_ms = std::chrono::duration<double, std::milli>(idx_end - idx_start).count();
    double docs_per_sec = (corpus_size / (idx_time_ms / 1000.0));

    auto stats = index.get_stats();
    std::cout << "[Step 2] Inverted Index Construction:\n"
              << "  - Indexing time: " << idx_time_ms << " ms (" << docs_per_sec << " docs/sec)\n"
              << "  - Unique terms: " << stats.total_unique_terms << "\n"
              << "  - Total postings: " << stats.total_postings << "\n"
              << "  - Total tokens: " << stats.total_token_occurrences << "\n"
              << "  - Avg document length: " << stats.avg_doc_length << " tokens\n"
              << "  - Estimated memory usage: " << (stats.estimated_memory_bytes / 1024.0 / 1024.0) << " MB\n";

    // 3. Index Persistence and Disk Size
    std::string index_path = "benchmark_" + std::to_string(corpus_size) + ".forge";
    auto save_res = IndexPersistence::save_to_file(index, index_path);
    if (save_res.success) {
        std::cout << "[Step 3] Index Persistence:\n"
                  << "  - Serialized file size: " << (save_res.bytes_written / 1024.0 / 1024.0) << " MB\n"
                  << "  - CRC32 Checksum: 0x" << std::hex << save_res.checksum << std::dec << "\n";
    }

    // Benchmark Queries
    std::vector<std::string> test_queries = {
        "procurement",
        "federal cybersecurity",
        "public AND tender",
        "compliance OR regulation",
        "infrastructure AND NOT legacy",
        "modernization architecture",
        "contract disclosure",
        "\"procurement regulation\"",
        "energy transportation",
        "bidding governance"
    };

    QueryParser parser;
    RetrievalEngine retrieval(index);

    // 4. Inverted Index: BM25 Latency
    std::cout << "[Step 4] Running Inverted Index BM25 Benchmark..." << std::endl;
    const size_t num_runs = 200;
    std::vector<double> bm25_latencies;
    bm25_latencies.reserve(num_runs);

    for (size_t i = 0; i < num_runs; ++i) {
        const auto& q_str = test_queries[i % test_queries.size()];
        auto parsed = parser.parse(q_str);
        auto t0 = std::chrono::high_resolution_clock::now();
        auto hits = retrieval.search(parsed, RankingAlgorithm::BM25);
        auto t1 = std::chrono::high_resolution_clock::now();
        bm25_latencies.push_back(std::chrono::duration<double, std::milli>(t1 - t0).count());
    }
    auto bm25_stats = compute_latency_stats(bm25_latencies);

    // 5. Inverted Index: TF-IDF Latency
    std::cout << "[Step 5] Running Inverted Index TF-IDF Benchmark..." << std::endl;
    std::vector<double> tfidf_latencies;
    tfidf_latencies.reserve(num_runs);
    for (size_t i = 0; i < num_runs; ++i) {
        const auto& q_str = test_queries[i % test_queries.size()];
        auto parsed = parser.parse(q_str);
        auto t0 = std::chrono::high_resolution_clock::now();
        auto hits = retrieval.search(parsed, RankingAlgorithm::TF_IDF);
        auto t1 = std::chrono::high_resolution_clock::now();
        tfidf_latencies.push_back(std::chrono::duration<double, std::milli>(t1 - t0).count());
    }
    auto tfidf_stats = compute_latency_stats(tfidf_latencies);

    // 6. Linear Document Scan Baseline (on smaller sample if size > 10,000 to keep run time reasonable)
    LinearScanner linear_scanner;
    for (const auto& doc : corpus) {
        linear_scanner.add_document(doc);
    }

    std::vector<double> linear_latencies;
    size_t linear_runs = 20;
    if (corpus_size >= 50000) linear_runs = 2;
    else if (corpus_size >= 10000) linear_runs = 5;

    std::cout << "[Step 6] Running Linear Scan Baseline (" << linear_runs << " iterations)..." << std::endl;
    for (size_t i = 0; i < linear_runs; ++i) {
        const auto& q_str = test_queries[i % test_queries.size()];
        auto t0 = std::chrono::high_resolution_clock::now();
        auto hits = linear_scanner.search(q_str, 20);
        auto t1 = std::chrono::high_resolution_clock::now();
        linear_latencies.push_back(std::chrono::duration<double, std::milli>(t1 - t0).count());
    }
    auto linear_stats = compute_latency_stats(linear_latencies);

    // 7. Concurrent Throughput Benchmark
    const int num_threads = 4;
    const size_t queries_per_thread = 250;
    std::atomic<size_t> total_concurrent_queries{0};

    auto conc_start = std::chrono::high_resolution_clock::now();
    std::vector<std::thread> workers;
    for (int t = 0; t < num_threads; ++t) {
        workers.emplace_back([&, t]() {
            for (size_t i = 0; i < queries_per_thread; ++i) {
                const auto& q_str = test_queries[(t + i) % test_queries.size()];
                auto parsed = parser.parse(q_str);
                auto hits = retrieval.search(parsed, RankingAlgorithm::BM25);
                total_concurrent_queries++;
            }
        });
    }
    for (auto& w : workers) w.join();
    auto conc_end = std::chrono::high_resolution_clock::now();
    double conc_time_sec = std::chrono::duration<double>(conc_end - conc_start).count();
    double concurrent_qps = total_concurrent_queries.load() / conc_time_sec;

    std::cout << "\n--- Search Latency & Performance Comparison ---\n";
    std::cout << std::left << std::setw(28) << "Metric"
              << std::setw(18) << "Linear Scan"
              << std::setw(18) << "Inverted (BM25)"
              << std::setw(18) << "Inverted (TF-IDF)" << "\n";
    std::cout << std::string(82, '-') << "\n";

    auto print_row = [](const std::string& label, double v1, double v2, double v3, const std::string& unit) {
        std::cout << std::left << std::setw(28) << label
                  << std::setw(18) << (std::to_string(v1) + " " + unit)
                  << std::setw(18) << (std::to_string(v2) + " " + unit)
                  << std::setw(18) << (std::to_string(v3) + " " + unit) << "\n";
    };

    print_row("Latency Mean (ms)", linear_stats.mean_ms, bm25_stats.mean_ms, tfidf_stats.mean_ms, "ms");
    print_row("Latency p50 (ms)", linear_stats.p50_ms, bm25_stats.p50_ms, tfidf_stats.p50_ms, "ms");
    print_row("Latency p95 (ms)", linear_stats.p95_ms, bm25_stats.p95_ms, tfidf_stats.p95_ms, "ms");
    print_row("Latency p99 (ms)", linear_stats.p99_ms, bm25_stats.p99_ms, tfidf_stats.p99_ms, "ms");
    print_row("Throughput (QPS)", linear_stats.qps, bm25_stats.qps, tfidf_stats.qps, "qps");

    double speedup = linear_stats.mean_ms / (bm25_stats.mean_ms > 0.0001 ? bm25_stats.mean_ms : 0.0001);
    std::cout << "\n>>> Inverted Index Speedup vs Linear Scan: " << std::fixed << std::setprecision(1)
              << speedup << "x FASTER\n";
    std::cout << ">>> Multi-threaded Read Throughput (4 threads): " << std::fixed << std::setprecision(1)
              << concurrent_qps << " QPS\n";

    std::remove(index_path.c_str());
}

int main(int argc, char* argv[]) {
    std::cout << "IndexForge — Performance Evaluation & Scaling Benchmark\n";
    std::cout << "Hardware / Compiler: x86_64 Clang++ 23.1, C++20, LLVM-MinGW UCRT\n";

    std::vector<size_t> sizes = {1000, 10000, 50000};
    if (argc > 1) {
        sizes.clear();
        for (int i = 1; i < argc; ++i) {
            sizes.push_back(std::stoul(argv[i]));
        }
    }

    for (size_t s : sizes) {
        run_benchmark_suite(s);
    }

    return 0;
}
