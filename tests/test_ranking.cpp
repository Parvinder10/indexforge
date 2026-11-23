#include "test_framework.hpp"
#include "indexforge/ranking.hpp"
#include "indexforge/inverted_index.hpp"
#include "indexforge/retrieval_engine.hpp"
#include "indexforge/query_parser.hpp"

using namespace indexforge;

TEST_CASE("Ranking - BM25 mathematical properties and parameter influence") {
    // 1. IDF should decrease as document frequency increases
    double idf_rare = RankingEngine::compute_bm25_idf(1000, 5);
    double idf_common = RankingEngine::compute_bm25_idf(1000, 500);
    ASSERT_GT(idf_rare, idf_common);

    // 2. Term frequency saturation: score(tf=10) should be less than 2 * score(tf=5)
    BM25Params params{.k1 = 1.2, .b = 0.75};
    double score_5 = RankingEngine::compute_bm25_term_score(idf_rare, 5, 100, 100.0, params);
    double score_10 = RankingEngine::compute_bm25_term_score(idf_rare, 10, 100, 100.0, params);
    ASSERT_GT(score_10, score_5);
    ASSERT_TRUE(score_10 < (2.0 * score_5)); // saturation test

    // 3. Document length normalization: for identical TF, shorter doc scores higher when b > 0
    double score_short = RankingEngine::compute_bm25_term_score(idf_rare, 3, 50, 100.0, params);
    double score_long = RankingEngine::compute_bm25_term_score(idf_rare, 3, 200, 100.0, params);
    ASSERT_GT(score_short, score_long);

    // 4. When b = 0, document length normalization is disabled
    BM25Params params_b0{.k1 = 1.2, .b = 0.0};
    double score_len_indep_short = RankingEngine::compute_bm25_term_score(idf_rare, 3, 50, 100.0, params_b0);
    double score_len_indep_long = RankingEngine::compute_bm25_term_score(idf_rare, 3, 200, 100.0, params_b0);
    ASSERT_NEAR(score_len_indep_short, score_len_indep_long, 1e-6);
}

TEST_CASE("Ranking - TF-IDF calculation correctness") {
    double idf = RankingEngine::compute_tfidf_idf(100, 10);
    ASSERT_GT(idf, 0.0);

    double score1 = RankingEngine::compute_tfidf_term_score(idf, 1, 10);
    double score4 = RankingEngine::compute_tfidf_term_score(idf, 4, 10);
    ASSERT_GT(score4, score1);
}

TEST_CASE("Ranking - Known query/document ranking verification") {
    InvertedIndex index;

    // Doc 1: Very concise, high density of "security"
    index.add_document(Document{
        .id = 1,
        .title = "Security Mandate",
        .content = "security security compliance"
    });

    // Doc 2: Long document with only 1 mention of "security"
    index.add_document(Document{
        .id = 2,
        .title = "General Government Operations Report",
        .content = "general administration government logistics transit infrastructure operations security budget"
    });

    QueryParser parser;
    auto parsed = parser.parse("security");
    RetrievalEngine retrieval(index);

    auto bm25_hits = retrieval.search(parsed, RankingAlgorithm::BM25);
    ASSERT_GE(bm25_hits.size(), 2);
    // Doc 1 must be ranked higher than Doc 2 due to higher term frequency and shorter document length
    ASSERT_EQ(bm25_hits[0].doc_id, 1);
    ASSERT_EQ(bm25_hits[1].doc_id, 2);
    ASSERT_GT(bm25_hits[0].score, bm25_hits[1].score);
}
