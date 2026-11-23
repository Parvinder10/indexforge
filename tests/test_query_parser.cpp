#include "test_framework.hpp"
#include "indexforge/query_parser.hpp"
#include "indexforge/inverted_index.hpp"
#include "indexforge/retrieval_engine.hpp"

using namespace indexforge;

TEST_CASE("QueryParser - Boolean expressions AST generation") {
    QueryParser parser;

    // Single keyword
    auto q1 = parser.parse("tender");
    ASSERT_TRUE(q1.root != nullptr);
    ASSERT_EQ(static_cast<int>(q1.root->type()), static_cast<int>(QueryNodeType::TERM));

    // AND query
    auto q2 = parser.parse("federal AND procurement");
    ASSERT_TRUE(q2.root != nullptr);
    ASSERT_EQ(static_cast<int>(q2.root->type()), static_cast<int>(QueryNodeType::AND));

    // OR query
    auto q3 = parser.parse("notice OR tender");
    ASSERT_TRUE(q3.root != nullptr);
    ASSERT_EQ(static_cast<int>(q3.root->type()), static_cast<int>(QueryNodeType::OR));

    // Exact phrase query
    auto q4 = parser.parse("\"public tender\"");
    ASSERT_TRUE(q4.root != nullptr);
    ASSERT_EQ(static_cast<int>(q4.root->type()), static_cast<int>(QueryNodeType::PHRASE));

    // NOT query
    auto q5 = parser.parse("cloud AND NOT legacy");
    ASSERT_TRUE(q5.root != nullptr);
    ASSERT_EQ(static_cast<int>(q5.root->type()), static_cast<int>(QueryNodeType::AND));
}

TEST_CASE("RetrievalEngine - Phrase matching with token position verification") {
    InvertedIndex index;

    // Doc 1 has exact consecutive phrase "public tender"
    index.add_document(Document{
        .id = 1,
        .title = "Tender Notice",
        .content = "announcement regarding public tender procurement"
    });

    // Doc 2 has both words, but separated ("public ... tender")
    index.add_document(Document{
        .id = 2,
        .title = "Public Sector Report",
        .content = "public policy on enterprise infrastructure with competitive tender"
    });

    QueryParser parser;
    RetrievalEngine retrieval(index);

    // Exact phrase search: "public tender"
    auto phrase_query = parser.parse("\"public tender\"");
    auto hits = retrieval.search(phrase_query);

    // Only Doc 1 matches exact consecutive phrase!
    ASSERT_EQ(hits.size(), 1);
    ASSERT_EQ(hits[0].doc_id, 1);
}

TEST_CASE("RetrievalEngine - Boolean AND, OR, NOT execution") {
    InvertedIndex index;

    index.add_document(Document{.id = 1, .title = "Doc 1", .content = "federal tender contract"});
    index.add_document(Document{.id = 2, .title = "Doc 2", .content = "state tender grant"});
    index.add_document(Document{.id = 3, .title = "Doc 3", .content = "federal regulation policy"});

    QueryParser parser;
    RetrievalEngine retrieval(index);

    // AND query: federal AND tender -> only Doc 1
    auto and_query = parser.parse("federal AND tender");
    auto and_hits = retrieval.search(and_query);
    ASSERT_EQ(and_hits.size(), 1);
    ASSERT_EQ(and_hits[0].doc_id, 1);

    // OR query: grant OR policy -> Doc 2 and Doc 3
    auto or_query = parser.parse("grant OR policy");
    auto or_hits = retrieval.search(or_query);
    ASSERT_EQ(or_hits.size(), 2);

    // NOT query: tender AND NOT state -> Doc 1
    auto not_query = parser.parse("tender AND NOT state");
    auto not_hits = retrieval.search(not_query);
    ASSERT_EQ(not_hits.size(), 1);
    ASSERT_EQ(not_hits[0].doc_id, 1);
}
