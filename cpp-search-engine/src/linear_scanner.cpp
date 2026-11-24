#include "indexforge/linear_scanner.hpp"
#include <algorithm>
#include <queue>
#include <unordered_set>

namespace indexforge {

LinearScanner::LinearScanner(Tokenizer tokenizer)
    : tokenizer_(std::move(tokenizer)) {}

void LinearScanner::add_document(Document doc) {
    documents_.push_back(std::move(doc));
}

void LinearScanner::clear() {
    documents_.clear();
}

std::vector<SearchHit> LinearScanner::search(
    const std::string& query,
    size_t limit) const {
    auto query_tokens = tokenizer_.tokenize(query);
    if (query_tokens.empty()) return {};

    std::unordered_set<std::string> target_terms;
    for (const auto& qt : query_tokens) {
        target_terms.insert(qt.text);
    }

    struct Candidate {
        Score score;
        const Document* doc;
        std::vector<std::string> matched_terms;

        bool operator>(const Candidate& other) const {
            return score > other.score;
        }
    };
    std::priority_queue<Candidate, std::vector<Candidate>, std::greater<Candidate>> min_heap;

    // Linear scan through EVERY document
    for (const auto& doc : documents_) {
        // Tokenize document on the fly
        auto doc_tokens = tokenizer_.tokenize(doc.content);
        
        std::unordered_map<std::string, uint32_t> tf_map;
        for (const auto& dt : doc_tokens) {
            if (target_terms.find(dt.text) != target_terms.end()) {
                tf_map[dt.text]++;
            }
        }

        if (tf_map.empty()) continue;

        Score score = 0.0;
        std::vector<std::string> matched;
        for (const auto& [term, count] : tf_map) {
            score += static_cast<Score>(count);
            matched.push_back(term);
        }

        Candidate c{
            .score = score,
            .doc = &doc,
            .matched_terms = std::move(matched)
        };

        if (min_heap.size() < limit) {
            min_heap.push(std::move(c));
        } else if (c.score > min_heap.top().score) {
            min_heap.pop();
            min_heap.push(std::move(c));
        }
    }

    std::vector<Candidate> sorted;
    while (!min_heap.empty()) {
        sorted.push_back(min_heap.top());
        min_heap.pop();
    }
    std::reverse(sorted.begin(), sorted.end());

    std::vector<SearchHit> hits;
    for (const auto& item : sorted) {
        hits.push_back(SearchHit{
            .doc_id = item.doc->id,
            .score = item.score,
            .title = item.doc->title,
            .source = item.doc->source,
            .date = item.doc->date,
            .doc_type = item.doc->doc_type,
            .snippet = "",
            .matched_terms = item.matched_terms,
            .document_length = item.doc->length
        });
    }

    return hits;
}

} // namespace indexforge
