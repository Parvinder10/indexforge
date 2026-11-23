#include "indexforge/ranking.hpp"
#include <algorithm>

namespace indexforge {

double RankingEngine::compute_bm25_idf(size_t total_docs, size_t doc_freq) {
    if (total_docs == 0 || doc_freq == 0) return 0.0;
    double N = static_cast<double>(total_docs);
    double n = static_cast<double>(doc_freq);
    
    // Lucene-style non-negative BM25 IDF: ln(1 + (N - n + 0.5) / (n + 0.5))
    double idf = std::log(1.0 + (N - n + 0.5) / (n + 0.5));
    return std::max(0.0, idf);
}

double RankingEngine::compute_bm25_term_score(
    double idf,
    uint32_t tf,
    uint32_t doc_len,
    double avg_doc_len,
    const BM25Params& params) {
    if (tf == 0 || idf <= 0.0) return 0.0;
    
    double dl = static_cast<double>(doc_len);
    double avgdl = avg_doc_len > 0.0 ? avg_doc_len : 1.0;
    double tf_d = static_cast<double>(tf);

    // Length normalization factor
    double K = params.k1 * ((1.0 - params.b) + params.b * (dl / avgdl));

    // Term saturation curve
    double tf_factor = (tf_d * (params.k1 + 1.0)) / (tf_d + K);

    return idf * tf_factor;
}

double RankingEngine::compute_tfidf_idf(size_t total_docs, size_t doc_freq) {
    if (total_docs == 0 || doc_freq == 0) return 0.0;
    double N = static_cast<double>(total_docs);
    double n = static_cast<double>(doc_freq);
    return 1.0 + std::log((N + 1.0) / (n + 1.0));
}

double RankingEngine::compute_tfidf_term_score(
    double idf,
    uint32_t tf,
    uint32_t doc_len) {
    if (tf == 0 || idf <= 0.0) return 0.0;
    
    // Sub-linear TF scaling: 1 + ln(tf)
    double tf_val = 1.0 + std::log(static_cast<double>(tf));
    
    // Cosine/length normalization: 1 / sqrt(max(doc_len, 1))
    double len_norm = 1.0 / std::sqrt(static_cast<double>(std::max(1u, doc_len)));

    return tf_val * idf * len_norm;
}

} // namespace indexforge
