#include "test_framework.hpp"
#include "indexforge/persistence.hpp"
#include "indexforge/inverted_index.hpp"
#include "indexforge/query_parser.hpp"
#include "indexforge/retrieval_engine.hpp"
#include <fstream>
#include <cstdio>

using namespace indexforge;

TEST_CASE("Persistence - Save, verify, and reload index") {
    const std::string test_file = "test_persist.forge";

    InvertedIndex original_index;
    original_index.add_document(Document{
        .id = 101,
        .title = "Federal Cloud Security Standard",
        .source = "NIST",
        .date = "2026-04-12",
        .doc_type = "regulation",
        .content = "mandatory cloud access security standard encryption protocols"
    });
    original_index.add_document(Document{
        .id = 102,
        .title = "Procurement Guidelines",
        .source = "OMB",
        .date = "2026-06-01",
        .doc_type = "policy",
        .content = "federal competitive bidding procurement procedures"
    });

    // Save
    auto save_res = IndexPersistence::save_to_file(original_index, test_file);
    ASSERT_TRUE(save_res.success);
    ASSERT_GT(save_res.bytes_written, 0);

    // Verify integrity
    auto verify_res = IndexPersistence::verify_integrity(test_file);
    ASSERT_TRUE(verify_res.success);
    ASSERT_EQ(verify_res.checksum, save_res.checksum);

    // Load into fresh index
    InvertedIndex loaded_index;
    auto load_res = IndexPersistence::load_from_file(loaded_index, test_file);
    ASSERT_TRUE(load_res.success);
    ASSERT_EQ(loaded_index.total_documents(), original_index.total_documents());
    ASSERT_EQ(loaded_index.total_unique_terms(), original_index.total_unique_terms());

    // Verify search works identically
    QueryParser parser;
    RetrievalEngine orig_retrieval(original_index);
    RetrievalEngine load_retrieval(loaded_index);

    auto query = parser.parse("cloud security");
    auto orig_hits = orig_retrieval.search(query);
    auto load_hits = load_retrieval.search(query);

    ASSERT_EQ(load_hits.size(), orig_hits.size());
    ASSERT_EQ(load_hits[0].doc_id, orig_hits[0].doc_id);
    ASSERT_NEAR(load_hits[0].score, orig_hits[0].score, 1e-5);

    std::remove(test_file.c_str());
}

TEST_CASE("Persistence - Corrupted and invalid file detection") {
    const std::string corrupt_file = "test_corrupt.forge";

    // Write invalid magic bytes
    std::ofstream out(corrupt_file, std::ios::binary);
    uint32_t bad_magic = 0xDEADBEEF;
    out.write(reinterpret_cast<const char*>(&bad_magic), 4);
    for (int i = 0; i < 30; ++i) out.put(0);
    out.close();

    auto verify_res = IndexPersistence::verify_integrity(corrupt_file);
    ASSERT_FALSE(verify_res.success);

    // Truncated file
    std::ofstream out_trunc(corrupt_file, std::ios::binary);
    out_trunc.write("IDXF", 4);
    out_trunc.close();

    auto trunc_res = IndexPersistence::verify_integrity(corrupt_file);
    ASSERT_FALSE(trunc_res.success);

    std::remove(corrupt_file.c_str());
}
