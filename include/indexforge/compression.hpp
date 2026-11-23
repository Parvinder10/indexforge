#pragma once

#include "indexforge/posting.hpp"
#include <vector>
#include <cstdint>
#include <span>

namespace indexforge {

class PostingCompressor {
public:
    // Encode single 32-bit unsigned integer using 7-bit variable-byte encoding
    static void encode_varint(uint32_t val, std::vector<uint8_t>& dest);

    // Decode single 32-bit unsigned integer from byte stream
    static bool decode_varint(const uint8_t*& ptr, const uint8_t* end, uint32_t& val);

    // Compress an entire PostingList (Delta-encoded doc_ids and positions + VByte)
    static std::vector<uint8_t> compress(const PostingList& postings);

    // Decompress byte stream back to PostingList
    static PostingList decompress(std::span<const uint8_t> data);

    // Memory savings statistics
    static size_t raw_memory_size(const PostingList& postings);
    static size_t compressed_memory_size(const std::vector<uint8_t>& compressed);
};

} // namespace indexforge
