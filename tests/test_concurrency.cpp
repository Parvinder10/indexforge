#include "test_framework.hpp"
#include "indexforge/inverted_index.hpp"
#include "indexforge/query_parser.hpp"
#include "indexforge/retrieval_engine.hpp"
#include <thread>
#include <vector>
#include <atomic>

using namespace indexforge;

TEST_CASE("Concurrency - Multi-threaded concurrent read-only queries") {
    InvertedIndex index;
    for (uint32_t i = 1; i <= 100; ++i) {
        index.add_document(Document{
            .id = i,
            .title = "Standard Notice " + std::to_string(i),
            .content = "federal compliance regulation cybersecurity standard tender notice " + std::to_string(i % 5)
        });
    }

    QueryParser parser;
    RetrievalEngine retrieval(index);
    std::atomic<size_t> total_queries{0};
    std::atomic<bool> failed{false};

    auto search_worker = [&](int thread_id) {
        std::vector<std::string> queries = {"federal tender", "cybersecurity standard", "compliance", "regulation"};
        for (int iter = 0; iter < 100; ++iter) {
            std::string q = queries[(thread_id + iter) % queries.size()];
            auto parsed = parser.parse(q);
            auto hits = retrieval.search(parsed);
            if (hits.empty()) {
                failed = true;
            }
            total_queries++;
        }
    };

    const int num_threads = 4;
    std::vector<std::thread> threads;
    threads.reserve(num_threads);
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back(search_worker, t);
    }

    for (auto& th : threads) {
        th.join();
    }

    ASSERT_FALSE(failed.load());
    ASSERT_EQ(total_queries.load(), 400u);
}

TEST_CASE("Concurrency - Concurrent search during incremental ingestion") {
    InvertedIndex index;
    for (uint32_t i = 1; i <= 50; ++i) {
        index.add_document(Document{
            .id = i,
            .title = "Initial Document " + std::to_string(i),
            .content = "public infrastructure tender bidding"
        });
    }

    QueryParser parser;
    RetrievalEngine retrieval(index);
    std::atomic<bool> stop_flag{false};
    std::atomic<size_t> successful_searches{0};

    // Reader thread
    std::thread reader([&]() {
        while (!stop_flag.load()) {
            auto parsed = parser.parse("infrastructure tender");
            auto hits = retrieval.search(parsed);
            if (!hits.empty()) {
                successful_searches++;
            }
            std::this_thread::yield();
        }
    });

    // Ingestion writer thread
    std::thread writer([&]() {
        for (uint32_t i = 51; i <= 100; ++i) {
            index.add_document(Document{
                .id = i,
                .title = "Incremental Doc " + std::to_string(i),
                .content = "infrastructure tender update " + std::to_string(i)
            });
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

    writer.join();
    stop_flag = true;
    reader.join();

    ASSERT_GT(successful_searches.load(), 0);
    ASSERT_EQ(index.active_documents(), 100);
}
