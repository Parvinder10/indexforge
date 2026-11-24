#pragma once

#include "indexforge/types.hpp"
#include "indexforge/inverted_index.hpp"
#include "indexforge/query_parser.hpp"
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace indexforge {

struct MatchInfo {
    uint32_t tf{0};
    std::vector<Position> positions;
};

// Candidate document matches: doc_id -> (term -> MatchInfo)
using DocMatches = std::unordered_map<DocId, std::unordered_map<std::string, MatchInfo>>;

class RetrievalEngine {
public:
    explicit RetrievalEngine(const InvertedIndex& index);

    // Execute query and return top-K ranked hits
    std::vector<SearchHit> search(
        const ParsedQuery& query,
        RankingAlgorithm ranking_algo = RankingAlgorithm::BM25,
        const BM25Params& bm25_params = BM25Params{},
        size_t limit = 20,
        size_t offset = 0) const;

    // Evaluate boolean expression to find matching doc IDs and their term frequencies
    std::unordered_set<DocId> evaluate_node(
        const QueryNode* node,
        DocMatches& matches) const;

private:
    const InvertedIndex& index_;

    // Evaluates phrase positions to verify exact phrase match
    static bool verify_phrase_postings(
        DocId doc_id,
        const std::vector<const PostingList*>& term_postings,
        uint32_t& phrase_tf);
};

} // namespace indexforge
