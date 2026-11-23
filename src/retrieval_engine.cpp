#include "indexforge/retrieval_engine.hpp"
#include "indexforge/ranking.hpp"
#include <queue>
#include <algorithm>

namespace indexforge {

RetrievalEngine::RetrievalEngine(const InvertedIndex& index)
    : index_(index) {}

bool RetrievalEngine::verify_phrase_postings(
    DocId doc_id,
    const std::vector<const PostingList*>& term_postings,
    uint32_t& phrase_tf) {
    phrase_tf = 0;
    if (term_postings.empty()) return false;

    // Collect position vectors for each term in the document
    std::vector<const std::vector<Position>*> term_positions;
    term_positions.reserve(term_postings.size());

    for (const auto* postings_ptr : term_postings) {
        if (!postings_ptr) return false;
        const auto& postings = *postings_ptr;
        auto it = std::lower_bound(postings.begin(), postings.end(), doc_id,
            [](const Posting& p, DocId id) { return p.doc_id < id; });
        if (it == postings.end() || it->doc_id != doc_id) {
            return false; // Term not present in this document
        }
        term_positions.push_back(&it->positions);
    }

    // Check consecutive positions
    const auto& first_positions = *term_positions[0];
    for (Position p0 : first_positions) {
        bool match = true;
        for (size_t i = 1; i < term_positions.size(); ++i) {
            Position expected_pos = p0 + static_cast<Position>(i);
            const auto& cur_pos_list = *term_positions[i];
            if (!std::binary_search(cur_pos_list.begin(), cur_pos_list.end(), expected_pos)) {
                match = false;
                break;
            }
        }
        if (match) {
            phrase_tf++;
        }
    }

    return phrase_tf > 0;
}

std::unordered_set<DocId> RetrievalEngine::evaluate_node(
    const QueryNode* node,
    DocMatches& matches) const {
    if (!node) return {};

    switch (node->type()) {
        case QueryNodeType::TERM: {
            const auto* tn = static_cast<const TermNode*>(node);
            auto postings = index_.get_postings(tn->term);
            std::unordered_set<DocId> matched_docs;
            matched_docs.reserve(postings.size());
            for (const auto& p : postings) {
                matched_docs.insert(p.doc_id);
                auto& match_info = matches[p.doc_id][tn->term];
                match_info.tf = p.term_frequency;
                match_info.positions = p.positions;
            }
            return matched_docs;
        }

        case QueryNodeType::PHRASE: {
            const auto* pn = static_cast<const PhraseNode*>(node);
            if (pn->terms.empty()) return {};

            // Fetch postings for each term ONCE
            std::vector<PostingList> term_postings_storage;
            term_postings_storage.reserve(pn->terms.size());
            std::vector<const PostingList*> term_postings_ptrs;
            term_postings_ptrs.reserve(pn->terms.size());

            for (const auto& term : pn->terms) {
                term_postings_storage.push_back(index_.get_postings(term));
                term_postings_ptrs.push_back(&term_postings_storage.back());
                if (term_postings_storage.back().empty()) {
                    return {}; // Term doesn't appear in corpus
                }
            }

            // Start with docs containing the first term
            std::unordered_set<DocId> candidate_docs;
            candidate_docs.reserve(term_postings_storage[0].size());
            for (const auto& p : term_postings_storage[0]) {
                candidate_docs.insert(p.doc_id);
            }

            // Intersect with remaining terms
            for (size_t i = 1; i < term_postings_storage.size(); ++i) {
                const auto& cur_postings = term_postings_storage[i];
                std::unordered_set<DocId> intersected;
                for (DocId doc_id : candidate_docs) {
                    auto it = std::lower_bound(cur_postings.begin(), cur_postings.end(), doc_id,
                        [](const Posting& p, DocId id) { return p.doc_id < id; });
                    if (it != cur_postings.end() && it->doc_id == doc_id) {
                        intersected.insert(doc_id);
                    }
                }
                candidate_docs = std::move(intersected);
                if (candidate_docs.empty()) break;
            }

            // Verify position adjacency
            std::unordered_set<DocId> phrase_matched_docs;
            std::string full_phrase;
            for (size_t i = 0; i < pn->terms.size(); ++i) {
                if (i > 0) full_phrase += " ";
                full_phrase += pn->terms[i];
            }

            for (DocId doc_id : candidate_docs) {
                uint32_t ptf = 0;
                if (verify_phrase_postings(doc_id, term_postings_ptrs, ptf)) {
                    phrase_matched_docs.insert(doc_id);
                    matches[doc_id][full_phrase] = MatchInfo{
                        .tf = ptf,
                        .positions = {}
                    };
                    for (const auto& t : pn->terms) {
                        matches[doc_id][t].tf += ptf;
                    }
                }
            }
            return phrase_matched_docs;
        }

        case QueryNodeType::AND: {
            const auto* an = static_cast<const AndNode*>(node);
            if (an->children.empty()) return {};

            // Evaluate positive and negative children
            std::vector<std::shared_ptr<QueryNode>> positive_children;
            std::vector<std::shared_ptr<QueryNode>> negative_children;

            for (const auto& c : an->children) {
                if (c->type() == QueryNodeType::NOT) {
                    negative_children.push_back(c);
                } else {
                    positive_children.push_back(c);
                }
            }

            if (positive_children.empty()) return {};

            // Start with first positive child
            std::unordered_set<DocId> result = evaluate_node(positive_children[0].get(), matches);

            for (size_t i = 1; i < positive_children.size(); ++i) {
                std::unordered_set<DocId> child_docs = evaluate_node(positive_children[i].get(), matches);
                std::unordered_set<DocId> intersected;
                for (DocId id : result) {
                    if (child_docs.find(id) != child_docs.end()) {
                        intersected.insert(id);
                    }
                }
                result = std::move(intersected);
                if (result.empty()) break;
            }

            // Subtract negative children
            for (const auto& neg : negative_children) {
                const auto* nn = static_cast<const NotNode*>(neg.get());
                DocMatches neg_matches;
                std::unordered_set<DocId> excluded = evaluate_node(nn->child.get(), neg_matches);
                for (DocId id : excluded) {
                    result.erase(id);
                }
            }

            return result;
        }

        case QueryNodeType::OR: {
            const auto* on = static_cast<const OrNode*>(node);
            std::unordered_set<DocId> result;
            for (const auto& child : on->children) {
                if (child->type() == QueryNodeType::NOT) continue;
                std::unordered_set<DocId> child_docs = evaluate_node(child.get(), matches);
                result.insert(child_docs.begin(), child_docs.end());
            }
            return result;
        }

        case QueryNodeType::NOT: {
            // Standalone NOT in top level
            return {};
        }
    }
    return {};
}

std::vector<SearchHit> RetrievalEngine::search(
    const ParsedQuery& query,
    RankingAlgorithm ranking_algo,
    const BM25Params& bm25_params,
    size_t limit,
    size_t offset) const {
    if (!query.root) return {};

    DocMatches matches;
    std::unordered_set<DocId> matched_docs = evaluate_node(query.root.get(), matches);
    if (matched_docs.empty()) return {};

    size_t total_docs = index_.active_documents();
    double avg_doc_len = index_.avg_doc_length();

    // Precompute IDF for all query terms ONCE upfront (O(|Q|) instead of O(|matched_docs| * |Q|))
    std::unordered_map<std::string, double> idf_cache;
    for (const auto& term : query.positive_terms) {
        size_t df = index_.get_doc_frequency(term);
        if (df == 0) df = 1;
        if (ranking_algo == RankingAlgorithm::BM25) {
            idf_cache[term] = RankingEngine::compute_bm25_idf(total_docs, df);
        } else {
            idf_cache[term] = RankingEngine::compute_tfidf_idf(total_docs, df);
        }
    }

    // Min-heap for top K results: elements are (Score, SearchHit)
    struct RankedEntry {
        Score score;
        DocId doc_id;
        std::vector<std::string> matched_terms;

        bool operator>(const RankedEntry& other) const {
            return score > other.score;
        }
    };
    std::priority_queue<RankedEntry, std::vector<RankedEntry>, std::greater<RankedEntry>> min_heap;

    for (DocId doc_id : matched_docs) {
        auto doc_meta = index_.get_document_meta(doc_id);
        if (!doc_meta) continue;

        uint32_t doc_len = doc_meta->length;
        Score total_score = 0.0;
        std::vector<std::string> terms_in_doc;

        const auto& doc_terms_map = matches[doc_id];
        for (const auto& [term, match_info] : doc_terms_map) {
            terms_in_doc.push_back(term);
            
            auto idf_it = idf_cache.find(term);
            double idf = 1.0;
            if (idf_it != idf_cache.end()) {
                idf = idf_it->second;
            } else {
                size_t df = index_.get_doc_frequency(term);
                if (df == 0) df = 1;
                idf = (ranking_algo == RankingAlgorithm::BM25)
                    ? RankingEngine::compute_bm25_idf(total_docs, df)
                    : RankingEngine::compute_tfidf_idf(total_docs, df);
                idf_cache[term] = idf;
            }

            if (ranking_algo == RankingAlgorithm::BM25) {
                double term_score = RankingEngine::compute_bm25_term_score(
                    idf, match_info.tf, doc_len, avg_doc_len, bm25_params);
                total_score += term_score;
            } else {
                double term_score = RankingEngine::compute_tfidf_term_score(
                    idf, match_info.tf, doc_len);
                total_score += term_score;
            }
        }

        if (total_score <= 0.0) continue;

        RankedEntry entry{
            .score = total_score,
            .doc_id = doc_id,
            .matched_terms = std::move(terms_in_doc)
        };

        if (min_heap.size() < (offset + limit)) {
            min_heap.push(std::move(entry));
        } else if (entry.score > min_heap.top().score) {
            min_heap.pop();
            min_heap.push(std::move(entry));
        }
    }

    // Extract from heap in descending order
    std::vector<RankedEntry> sorted_entries;
    sorted_entries.reserve(min_heap.size());
    while (!min_heap.empty()) {
        sorted_entries.push_back(min_heap.top());
        min_heap.pop();
    }
    std::reverse(sorted_entries.begin(), sorted_entries.end());

    // Apply offset and limit
    std::vector<SearchHit> hits;
    for (size_t i = offset; i < sorted_entries.size() && hits.size() < limit; ++i) {
        const auto& entry = sorted_entries[i];
        auto doc = index_.get_document(entry.doc_id);
        if (!doc) continue;

        hits.push_back(SearchHit{
            .doc_id = entry.doc_id,
            .score = entry.score,
            .title = doc->title,
            .source = doc->source,
            .date = doc->date,
            .doc_type = doc->doc_type,
            .snippet = "", // SnippetGenerator will populate this
            .matched_terms = entry.matched_terms,
            .document_length = doc->length
        });
    }

    return hits;
}

} // namespace indexforge
