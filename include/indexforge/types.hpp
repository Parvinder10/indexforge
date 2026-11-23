#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>

namespace indexforge {

using DocId = uint32_t;
using TermId = uint32_t;
using Position = uint32_t;
using Score = double;

constexpr DocId INVALID_DOC_ID = 0xFFFFFFFF;

enum class RankingAlgorithm {
    BM25,
    TF_IDF
};

struct BM25Params {
    double k1{1.2};
    double b{0.75};
};

struct Token {
    std::string text;
    Position position{0};
    size_t start_offset{0};
    size_t end_offset{0};

    bool operator==(const Token& other) const {
        return text == other.text && position == other.position &&
               start_offset == other.start_offset && end_offset == other.end_offset;
    }
};

struct SearchHit {
    DocId doc_id{0};
    Score score{0.0};
    std::string title;
    std::string source;
    std::string date;
    std::string doc_type;
    std::string snippet;
    std::vector<std::string> matched_terms;
    uint32_t document_length{0};
};

struct SearchResult {
    std::vector<SearchHit> hits;
    size_t total_hits{0};
    double latency_ms{0.0};
    bool from_cache{false};
};

struct IndexStats {
    size_t total_documents{0};
    size_t active_documents{0};
    size_t deleted_documents{0};
    size_t total_unique_terms{0};
    size_t total_postings{0};
    size_t total_token_occurrences{0};
    double avg_doc_length{0.0};
    size_t estimated_memory_bytes{0};
    size_t cache_entries{0};
    size_t cache_hits{0};
    size_t cache_misses{0};
};

} // namespace indexforge
