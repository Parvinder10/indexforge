#include "test_framework.hpp"
#include "indexforge/inverted_index.hpp"

using namespace indexforge;

TEST_CASE("InvertedIndex - Document ingestion and term frequency") {
    InvertedIndex index;

    Document doc1{
        .id = 1,
        .title = "Procurement Notice",
        .source = "GSA",
        .date = "2026-01-01",
        .doc_type = "notice",
        .content = "tender procurement tender notice federal"
    };

    bool ok = index.add_document(doc1);
    ASSERT_TRUE(ok);
    ASSERT_EQ(index.total_documents(), 1);
    ASSERT_EQ(index.active_documents(), 1);

    // Stem of "tender" is "tender"
    auto postings = index.get_postings("tender");
    ASSERT_EQ(postings.size(), 1);
    ASSERT_EQ(postings[0].doc_id, 1);
    ASSERT_EQ(postings[0].term_frequency, 2);
    ASSERT_EQ(postings[0].positions.size(), 2);
}

TEST_CASE("InvertedIndex - Document deletion and tombstoning") {
    InvertedIndex index;

    index.add_document(Document{.id = 1, .title = "Doc 1", .content = "cybersecurity compliance"});
    index.add_document(Document{.id = 2, .title = "Doc 2", .content = "cybersecurity audit"});

    ASSERT_EQ(index.active_documents(), 2);
    ASSERT_EQ(index.get_postings("audit").size(), 1);

    // Delete doc 2
    bool deleted = index.delete_document(2);
    ASSERT_TRUE(deleted);
    ASSERT_EQ(index.active_documents(), 1);
    ASSERT_EQ(index.deleted_documents(), 1);

    // Active postings for "audit" should now be 0
    ASSERT_EQ(index.get_postings("audit").size(), 0);

    // Active postings for "cybersecurity" (stem: cybersecurity) should only return doc 1
    auto cyber_postings = index.get_postings(index.tokenizer().process_term("cybersecurity"));
    ASSERT_EQ(cyber_postings.size(), 1);
    ASSERT_EQ(cyber_postings[0].doc_id, 1);
}

TEST_CASE("InvertedIndex - Document update and replacement") {
    InvertedIndex index;

    index.add_document(Document{.id = 1, .title = "Draft", .content = "preliminary draft specifications"});
    auto meta1 = index.get_document_meta(1);
    ASSERT_TRUE(meta1.has_value());
    ASSERT_EQ(meta1->title, "Draft");

    // Update doc 1
    index.update_document(Document{.id = 1, .title = "Final Policy", .content = "finalized approved regulation"});
    auto meta2 = index.get_document_meta(1);
    ASSERT_TRUE(meta2.has_value());
    ASSERT_EQ(meta2->title, "Final Policy");
    ASSERT_EQ(index.active_documents(), 1);
}

TEST_CASE("InvertedIndex - Empty and malformed documents") {
    InvertedIndex index;

    // Document with empty content
    bool ok1 = index.add_document(Document{.id = 1, .title = "Empty", .content = ""});
    ASSERT_TRUE(ok1);
    ASSERT_EQ(index.active_documents(), 1);
    ASSERT_EQ(index.get_document_meta(1)->length, 0);

    // Document with whitespace and symbols only
    bool ok2 = index.add_document(Document{.id = 2, .title = "Symbols", .content = "   !!! ??? ---   "});
    ASSERT_TRUE(ok2);
    ASSERT_EQ(index.get_document_meta(2)->length, 0);

    // Document with invalid ID
    bool ok3 = index.add_document(Document{.id = INVALID_DOC_ID, .title = "Invalid"});
    ASSERT_FALSE(ok3);
}

TEST_CASE("InvertedIndex - Compaction purges tombstones") {
    InvertedIndex index;

    index.add_document(Document{.id = 1, .title = "A", .content = "apple banana"});
    index.add_document(Document{.id = 2, .title = "B", .content = "apple orange"});
    index.delete_document(2);

    ASSERT_EQ(index.deleted_documents(), 1);
    index.compact();
    ASSERT_EQ(index.deleted_documents(), 0);
    ASSERT_EQ(index.total_documents(), 1);
    ASSERT_EQ(index.active_documents(), 1);
}
