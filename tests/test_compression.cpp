#include "test_framework.hpp"
#include "indexforge/compression.hpp"

using namespace indexforge;

TEST_CASE("Compression - Varint single integer encoding/decoding") {
    std::vector<uint8_t> buf;
    std::vector<uint32_t> test_values = {0, 1, 127, 128, 255, 300, 16383, 16384, 1000000, 0xFFFFFFFF};

    for (uint32_t v : test_values) {
        buf.clear();
        PostingCompressor::encode_varint(v, buf);
        const uint8_t* ptr = buf.data();
        const uint8_t* end = ptr + buf.size();
        uint32_t decoded = 0;
        bool ok = PostingCompressor::decode_varint(ptr, end, decoded);
        ASSERT_TRUE(ok);
        ASSERT_EQ(decoded, v);
    }
}

TEST_CASE("Compression - Postings list round trip with delta encoding") {
    PostingList original = {
        Posting{.doc_id = 10, .term_frequency = 2, .positions = {3, 15}},
        Posting{.doc_id = 15, .term_frequency = 1, .positions = {42}},
        Posting{.doc_id = 100, .term_frequency = 3, .positions = {1, 5, 99}},
        Posting{.doc_id = 105, .term_frequency = 1, .positions = {7}}
    };

    auto compressed = PostingCompressor::compress(original);
    ASSERT_GT(compressed.size(), 0);

    // Verify compressed size is smaller than raw struct size
    size_t raw_size = PostingCompressor::raw_memory_size(original);
    ASSERT_GT(raw_size, compressed.size());

    auto decompressed = PostingCompressor::decompress(compressed);
    ASSERT_EQ(decompressed.size(), original.size());

    for (size_t i = 0; i < original.size(); ++i) {
        ASSERT_EQ(decompressed[i].doc_id, original[i].doc_id);
        ASSERT_EQ(decompressed[i].term_frequency, original[i].term_frequency);
        ASSERT_EQ(decompressed[i].positions.size(), original[i].positions.size());
        for (size_t j = 0; j < original[i].positions.size(); ++j) {
            ASSERT_EQ(decompressed[i].positions[j], original[i].positions[j]);
        }
    }
}
