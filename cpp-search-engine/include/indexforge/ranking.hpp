#pragma once

#include "indexforge/types.hpp"
#include <cmath>

namespace indexforge {

class RankingEngine {
public:
    // Okapi BM25 Inverse Document Frequency
    // IDF(t) = ln( 1 + (N - n(t) + 0.5) / (n(t) + 0.5) )
    static double compute_bm25_idf(size_t total_docs, size_t doc_freq);

    // Okapi BM25 term weight
    // weight = IDF(t) * (tf * (k1 + 1)) / (tf + k1 * (1 - b + b * (doc_len / avgdl)))
    static double compute_bm25_term_score(
        double idf,
        uint32_t tf,
        uint32_t doc_len,
        double avg_doc_len,
        const BM25Params& params);

    // TF-IDF term weight
    // TF = 1 + ln(tf)
    // IDF = 1 + ln(N / (df + 1))
    // Score = TF * IDF / sqrt(doc_len)
    static double compute_tfidf_idf(size_t total_docs, size_t doc_freq);
    static double compute_tfidf_term_score(
        double idf,
        uint32_t tf,
        uint32_t doc_len);
};

} // namespace indexforge
