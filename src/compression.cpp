#include "indexforge/compression.hpp"

namespace indexforge {

void PostingCompressor::encode_varint(uint32_t val, std::vector<uint8_t>& dest) {
    while (val >= 0x80) {
        dest.push_back(static_cast<uint8_t>((val & 0x7F) | 0x80));
        val >>= 7;
    }
    dest.push_back(static_cast<uint8_t>(val & 0x7F));
}

bool PostingCompressor::decode_varint(const uint8_t*& ptr, const uint8_t* end, uint32_t& val) {
    val = 0;
    uint32_t shift = 0;
    while (ptr < end) {
        uint8_t byte = *ptr++;
        val |= static_cast<uint32_t>(byte & 0x7F) << shift;
        if (!(byte & 0x80)) {
            return true;
        }
        shift += 7;
        if (shift >= 35) {
            return false; // Overflow protection
        }
    }
    return false;
}

std::vector<uint8_t> PostingCompressor::compress(const PostingList& postings) {
    std::vector<uint8_t> out;
    if (postings.empty()) return out;

    // First write posting count
    encode_varint(static_cast<uint32_t>(postings.size()), out);

    DocId prev_doc_id = 0;
    for (size_t i = 0; i < postings.size(); ++i) {
        const auto& p = postings[i];
        
        // Delta encode doc ID: first is absolute, rest are d-gap
        uint32_t doc_delta = (i == 0) ? p.doc_id : (p.doc_id - prev_doc_id);
        encode_varint(doc_delta, out);
        prev_doc_id = p.doc_id;

        // Term frequency
        encode_varint(p.term_frequency, out);

        // Number of positions
        encode_varint(static_cast<uint32_t>(p.positions.size()), out);

        // Delta-encoded positions
        Position prev_pos = 0;
        for (size_t pos_idx = 0; pos_idx < p.positions.size(); ++pos_idx) {
            Position cur_pos = p.positions[pos_idx];
            Position pos_delta = (pos_idx == 0) ? cur_pos : (cur_pos - prev_pos);
            encode_varint(pos_delta, out);
            prev_pos = cur_pos;
        }
    }

    return out;
}

PostingList PostingCompressor::decompress(std::span<const uint8_t> data) {
    PostingList postings;
    if (data.empty()) return postings;

    const uint8_t* ptr = data.data();
    const uint8_t* end = ptr + data.size();

    uint32_t count = 0;
    if (!decode_varint(ptr, end, count)) return postings;
    postings.reserve(count);

    DocId prev_doc_id = 0;
    for (uint32_t i = 0; i < count; ++i) {
        uint32_t doc_delta = 0;
        if (!decode_varint(ptr, end, doc_delta)) break;
        DocId cur_doc_id = (i == 0) ? doc_delta : (prev_doc_id + doc_delta);
        prev_doc_id = cur_doc_id;

        uint32_t tf = 0;
        if (!decode_varint(ptr, end, tf)) break;

        uint32_t pos_count = 0;
        if (!decode_varint(ptr, end, pos_count)) break;

        std::vector<Position> positions;
        positions.reserve(pos_count);
        Position prev_pos = 0;
        for (uint32_t pos_idx = 0; pos_idx < pos_count; ++pos_idx) {
            Position pos_delta = 0;
            if (!decode_varint(ptr, end, pos_delta)) break;
            Position cur_pos = (pos_idx == 0) ? pos_delta : (prev_pos + pos_delta);
            positions.push_back(cur_pos);
            prev_pos = cur_pos;
        }

        postings.push_back(Posting{
            .doc_id = cur_doc_id,
            .term_frequency = tf,
            .positions = std::move(positions)
        });
    }

    return postings;
}

size_t PostingCompressor::raw_memory_size(const PostingList& postings) {
    size_t bytes = sizeof(PostingList) + postings.capacity() * sizeof(Posting);
    for (const auto& p : postings) {
        bytes += p.positions.capacity() * sizeof(Position);
    }
    return bytes;
}

size_t PostingCompressor::compressed_memory_size(const std::vector<uint8_t>& compressed) {
    return sizeof(std::vector<uint8_t>) + compressed.capacity();
}

} // namespace indexforge
