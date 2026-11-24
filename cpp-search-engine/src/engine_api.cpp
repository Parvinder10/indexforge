#define INDEXFORGE_EXPORTS
#include "indexforge/engine_api.h"
#include "indexforge/inverted_index.hpp"
#include "indexforge/retrieval_engine.hpp"
#include "indexforge/snippet_generator.hpp"
#include "indexforge/query_parser.hpp"
#include "indexforge/cache.hpp"
#include "indexforge/persistence.hpp"
#include "indexforge/linear_scanner.hpp"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <cstdlib>

using namespace indexforge;

struct IndexForgeEngine {
    Tokenizer tokenizer;
    InvertedIndex index;
    RetrievalEngine retrieval;
    SnippetGenerator snippet_gen;
    QueryParser parser;
    QueryCache cache;
    LinearScanner linear_scanner;

    IndexForgeEngine()
        : tokenizer(),
          index(tokenizer),
          retrieval(index),
          snippet_gen(tokenizer),
          parser(tokenizer),
          cache(2048),
          linear_scanner(tokenizer) {}
};

namespace {

std::string escape_json(std::string_view s) {
    std::string out;
    out.reserve(s.size() + 16);
    for (char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    std::ostringstream ss;
                    ss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
                    out += ss.str();
                } else {
                    out.push_back(c);
                }
                break;
        }
    }
    return out;
}

char* copy_to_c_str(const std::string& str) {
    char* c_str = static_cast<char*>(std::malloc(str.size() + 1));
    if (c_str) {
        std::memcpy(c_str, str.c_str(), str.size() + 1);
    }
    return c_str;
}

} // namespace

extern "C" {

indexforge_handle indexforge_create() {
    try {
        return new IndexForgeEngine();
    } catch (...) {
        return nullptr;
    }
}

void indexforge_destroy(indexforge_handle handle) {
    if (handle) {
        delete static_cast<IndexForgeEngine*>(handle);
    }
}

int indexforge_index_document(
    indexforge_handle handle,
    uint32_t doc_id,
    const char* title,
    const char* source,
    const char* date,
    const char* doc_type,
    const char* content) {
    if (!handle) return -1;
    auto* engine = static_cast<IndexForgeEngine*>(handle);

    Document doc{
        .id = doc_id,
        .title = title ? title : "",
        .source = source ? source : "",
        .date = date ? date : "",
        .doc_type = doc_type ? doc_type : "",
        .content = content ? content : ""
    };

    engine->linear_scanner.add_document(doc);
    bool ok = engine->index.add_document(std::move(doc));
    if (ok) {
        engine->cache.clear();
    }
    return ok ? 0 : -1;
}

int indexforge_delete_document(indexforge_handle handle, uint32_t doc_id) {
    if (!handle) return -1;
    auto* engine = static_cast<IndexForgeEngine*>(handle);
    bool ok = engine->index.delete_document(doc_id);
    if (ok) {
        engine->cache.clear();
    }
    return ok ? 0 : -1;
}

int indexforge_update_document(
    indexforge_handle handle,
    uint32_t doc_id,
    const char* title,
    const char* source,
    const char* date,
    const char* doc_type,
    const char* content) {
    return indexforge_index_document(handle, doc_id, title, source, date, doc_type, content);
}

const char* indexforge_search(
    indexforge_handle handle,
    const char* query,
    int ranking_algo,
    double k1,
    double b,
    uint32_t limit,
    uint32_t offset) {
    if (!handle || !query) return copy_to_c_str("{\"error\":\"invalid arguments\"}");
    auto* engine = static_cast<IndexForgeEngine*>(handle);

    std::string query_str(query);
    std::string cache_key = query_str + "_" + std::to_string(ranking_algo) + "_" +
                            std::to_string(k1) + "_" + std::to_string(b) + "_" +
                            std::to_string(limit) + "_" + std::to_string(offset);

    // Check cache
    auto cached = engine->cache.get(cache_key);
    if (cached) {
        std::ostringstream json;
        json << "{\"total_hits\":" << cached->total_hits
             << ",\"latency_ms\":" << std::fixed << std::setprecision(3) << cached->latency_ms
             << ",\"from_cache\":true"
             << ",\"hits\":[";
        for (size_t i = 0; i < cached->hits.size(); ++i) {
            const auto& h = cached->hits[i];
            if (i > 0) json << ",";
            json << "{\"doc_id\":" << h.doc_id
                 << ",\"score\":" << std::fixed << std::setprecision(4) << h.score
                 << ",\"title\":\"" << escape_json(h.title) << "\""
                 << ",\"source\":\"" << escape_json(h.source) << "\""
                 << ",\"date\":\"" << escape_json(h.date) << "\""
                 << ",\"doc_type\":\"" << escape_json(h.doc_type) << "\""
                 << ",\"snippet\":\"" << escape_json(h.snippet) << "\""
                 << ",\"doc_length\":" << h.document_length
                 << ",\"matched_terms\":[";
            for (size_t j = 0; j < h.matched_terms.size(); ++j) {
                if (j > 0) json << ",";
                json << "\"" << escape_json(h.matched_terms[j]) << "\"";
            }
            json << "]}";
        }
        json << "]}";
        return copy_to_c_str(json.str());
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    auto parsed = engine->parser.parse(query_str);
    RankingAlgorithm algo = (ranking_algo == 1) ? RankingAlgorithm::TF_IDF : RankingAlgorithm::BM25;
    BM25Params params{.k1 = k1, .b = b};

    auto hits = engine->retrieval.search(parsed, algo, params, limit, offset);

    // Generate snippets for returned hits
    for (auto& h : hits) {
        auto doc = engine->index.get_document(h.doc_id);
        if (doc) {
            h.snippet = engine->snippet_gen.generate_snippet(doc->content, h.matched_terms);
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    double latency = std::chrono::duration<double, std::milli>(end_time - start_time).count();

    SearchResult sr{
        .hits = hits,
        .total_hits = hits.size(),
        .latency_ms = latency,
        .from_cache = false
    };

    engine->cache.put(cache_key, sr);

    std::ostringstream json;
    json << "{\"total_hits\":" << sr.total_hits
         << ",\"latency_ms\":" << std::fixed << std::setprecision(3) << sr.latency_ms
         << ",\"from_cache\":false"
         << ",\"hits\":[";
    for (size_t i = 0; i < hits.size(); ++i) {
        const auto& h = hits[i];
        if (i > 0) json << ",";
        json << "{\"doc_id\":" << h.doc_id
             << ",\"score\":" << std::fixed << std::setprecision(4) << h.score
             << ",\"title\":\"" << escape_json(h.title) << "\""
             << ",\"source\":\"" << escape_json(h.source) << "\""
             << ",\"date\":\"" << escape_json(h.date) << "\""
             << ",\"doc_type\":\"" << escape_json(h.doc_type) << "\""
             << ",\"snippet\":\"" << escape_json(h.snippet) << "\""
             << ",\"doc_length\":" << h.document_length
             << ",\"matched_terms\":[";
        for (size_t j = 0; j < h.matched_terms.size(); ++j) {
            if (j > 0) json << ",";
            json << "\"" << escape_json(h.matched_terms[j]) << "\"";
        }
        json << "]}";
    }
    json << "]}";

    return copy_to_c_str(json.str());
}

const char* indexforge_linear_search(indexforge_handle handle, const char* query, uint32_t limit) {
    if (!handle || !query) return copy_to_c_str("{\"error\":\"invalid arguments\"}");
    auto* engine = static_cast<IndexForgeEngine*>(handle);

    auto start_time = std::chrono::high_resolution_clock::now();
    auto hits = engine->linear_scanner.search(query, limit);

    for (auto& h : hits) {
        auto doc = engine->index.get_document(h.doc_id);
        if (doc) {
            h.snippet = engine->snippet_gen.generate_snippet(doc->content, h.matched_terms);
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    double latency = std::chrono::duration<double, std::milli>(end_time - start_time).count();

    std::ostringstream json;
    json << "{\"total_hits\":" << hits.size()
         << ",\"latency_ms\":" << std::fixed << std::setprecision(3) << latency
         << ",\"hits\":[";
    for (size_t i = 0; i < hits.size(); ++i) {
        const auto& h = hits[i];
        if (i > 0) json << ",";
        json << "{\"doc_id\":" << h.doc_id
             << ",\"score\":" << std::fixed << std::setprecision(4) << h.score
             << ",\"title\":\"" << escape_json(h.title) << "\""
             << ",\"source\":\"" << escape_json(h.source) << "\""
             << ",\"date\":\"" << escape_json(h.date) << "\""
             << ",\"doc_type\":\"" << escape_json(h.doc_type) << "\""
             << ",\"snippet\":\"" << escape_json(h.snippet) << "\""
             << ",\"doc_length\":" << h.document_length
             << "}";
    }
    json << "]}";

    return copy_to_c_str(json.str());
}

const char* indexforge_get_stats(indexforge_handle handle) {
    if (!handle) return copy_to_c_str("{\"error\":\"null handle\"}");
    auto* engine = static_cast<IndexForgeEngine*>(handle);

    auto stats = engine->index.get_stats();
    stats.cache_entries = engine->cache.size();
    stats.cache_hits = engine->cache.hits();
    stats.cache_misses = engine->cache.misses();

    std::ostringstream json;
    json << "{"
         << "\"total_documents\":" << stats.total_documents
         << ",\"active_documents\":" << stats.active_documents
         << ",\"deleted_documents\":" << stats.deleted_documents
         << ",\"total_unique_terms\":" << stats.total_unique_terms
         << ",\"total_postings\":" << stats.total_postings
         << ",\"total_token_occurrences\":" << stats.total_token_occurrences
         << ",\"avg_doc_length\":" << std::fixed << std::setprecision(2) << stats.avg_doc_length
         << ",\"estimated_memory_bytes\":" << stats.estimated_memory_bytes
         << ",\"cache_entries\":" << stats.cache_entries
         << ",\"cache_hits\":" << stats.cache_hits
         << ",\"cache_misses\":" << stats.cache_misses
         << "}";

    return copy_to_c_str(json.str());
}

int indexforge_save_index(indexforge_handle handle, const char* filepath) {
    if (!handle || !filepath) return -1;
    auto* engine = static_cast<IndexForgeEngine*>(handle);
    auto res = IndexPersistence::save_to_file(engine->index, filepath);
    return res.success ? 0 : -1;
}

int indexforge_load_index(indexforge_handle handle, const char* filepath) {
    if (!handle || !filepath) return -1;
    auto* engine = static_cast<IndexForgeEngine*>(handle);
    auto res = IndexPersistence::load_from_file(engine->index, filepath);
    if (res.success) {
        engine->cache.clear();
        return 0;
    }
    return -1;
}

const char* indexforge_verify_integrity(const char* filepath) {
    if (!filepath) return copy_to_c_str("{\"valid\":false,\"error\":\"null path\"}");
    auto res = IndexPersistence::verify_integrity(filepath);
    std::ostringstream json;
    json << "{"
         << "\"valid\":" << (res.success ? "true" : "false")
         << ",\"checksum\":\"0x" << std::hex << res.checksum << std::dec << "\""
         << ",\"bytes_checked\":" << res.bytes_written;
    if (!res.success) {
        json << ",\"error\":\"" << escape_json(res.error_message) << "\"";
    }
    json << "}";
    return copy_to_c_str(json.str());
}

void indexforge_clear_cache(indexforge_handle handle) {
    if (handle) {
        static_cast<IndexForgeEngine*>(handle)->cache.clear();
    }
}

void indexforge_compact(indexforge_handle handle) {
    if (handle) {
        static_cast<IndexForgeEngine*>(handle)->index.compact();
    }
}

void indexforge_free_string(char* str) {
    if (str) {
        std::free(str);
    }
}

} // extern "C"
