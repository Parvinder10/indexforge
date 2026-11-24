#include "indexforge/persistence.hpp"
#include "indexforge/compression.hpp"
#include <fstream>
#include <vector>
#include <cstring>

namespace indexforge {

namespace {

// Fast CRC32 lookup table
class CRC32Table {
public:
    uint32_t table[256];
    CRC32Table() {
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t crc = i;
            for (uint32_t j = 0; j < 8; ++j) {
                crc = (crc & 1) ? (0xEDB88320 ^ (crc >> 1)) : (crc >> 1);
            }
            table[i] = crc;
        }
    }
};

static const CRC32Table g_crc32;

// Binary buffer serializer helpers
void write_u32(std::vector<uint8_t>& buf, uint32_t val) {
    buf.push_back(static_cast<uint8_t>(val & 0xFF));
    buf.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
    buf.push_back(static_cast<uint8_t>((val >> 16) & 0xFF));
    buf.push_back(static_cast<uint8_t>((val >> 24) & 0xFF));
}

void write_u64(std::vector<uint8_t>& buf, uint64_t val) {
    for (int i = 0; i < 8; ++i) {
        buf.push_back(static_cast<uint8_t>((val >> (i * 8)) & 0xFF));
    }
}

void write_string(std::vector<uint8_t>& buf, const std::string& str) {
    write_u32(buf, static_cast<uint32_t>(str.size()));
    buf.insert(buf.end(), str.begin(), str.end());
}

bool read_u32(const uint8_t*& ptr, const uint8_t* end, uint32_t& val) {
    if (ptr + 4 > end) return false;
    val = static_cast<uint32_t>(ptr[0]) |
          (static_cast<uint32_t>(ptr[1]) << 8) |
          (static_cast<uint32_t>(ptr[2]) << 16) |
          (static_cast<uint32_t>(ptr[3]) << 24);
    ptr += 4;
    return true;
}

bool read_u64(const uint8_t*& ptr, const uint8_t* end, uint64_t& val) {
    if (ptr + 8 > end) return false;
    val = 0;
    for (int i = 0; i < 8; ++i) {
        val |= (static_cast<uint64_t>(ptr[i]) << (i * 8));
    }
    ptr += 8;
    return true;
}

bool read_string(const uint8_t*& ptr, const uint8_t* end, std::string& str) {
    uint32_t len = 0;
    if (!read_u32(ptr, end, len)) return false;
    if (ptr + len > end) return false;
    str.assign(reinterpret_cast<const char*>(ptr), len);
    ptr += len;
    return true;
}

} // namespace

uint32_t IndexPersistence::calculate_crc32(const uint8_t* data, size_t length) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; ++i) {
        crc = g_crc32.table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    return ~crc;
}

PersistenceResult IndexPersistence::save_to_file(const InvertedIndex& index, const std::string& filepath) {
    std::shared_lock<std::shared_mutex> lock(index.mutex());

    std::vector<uint8_t> payload;

    // Index metadata
    write_u64(payload, static_cast<uint64_t>(index.total_token_count()));

    // Documents
    const auto& docs = index.doc_store();
    write_u32(payload, static_cast<uint32_t>(docs.size()));
    for (const auto& [id, doc] : docs) {
        write_u32(payload, id);
        write_string(payload, doc.title);
        write_string(payload, doc.source);
        write_string(payload, doc.date);
        write_string(payload, doc.doc_type);
        write_string(payload, doc.content);
        write_u32(payload, doc.length);
        write_u32(payload, doc.unique_terms);
    }

    // Deleted doc IDs
    const auto& deleted = index.deleted_set();
    write_u32(payload, static_cast<uint32_t>(deleted.size()));
    for (DocId id : deleted) {
        write_u32(payload, id);
    }

    // Terms and compressed postings
    const auto& raw_idx = index.raw_index();
    write_u32(payload, static_cast<uint32_t>(raw_idx.size()));
    for (const auto& [term, postings] : raw_idx) {
        write_string(payload, term);
        auto compressed = PostingCompressor::compress(postings);
        write_u32(payload, static_cast<uint32_t>(compressed.size()));
        payload.insert(payload.end(), compressed.begin(), compressed.end());
    }

    // Compute CRC32
    uint32_t crc = calculate_crc32(payload.data(), payload.size());

    // Write file with header: MAGIC(4), VERSION(4), CRC32(4), PAYLOAD_SIZE(8), PAYLOAD
    std::ofstream out(filepath, std::ios::binary);
    if (!out) {
        return PersistenceResult{
            .success = false,
            .error_message = "Failed to open output file: " + filepath
        };
    }

    std::vector<uint8_t> header;
    write_u32(header, MAGIC_HEADER);
    write_u32(header, FORMAT_VERSION);
    write_u32(header, crc);
    write_u64(header, static_cast<uint64_t>(payload.size()));

    out.write(reinterpret_cast<const char*>(header.data()), header.size());
    out.write(reinterpret_cast<const char*>(payload.data()), payload.size());
    out.close();

    return PersistenceResult{
        .success = true,
        .error_message = "",
        .bytes_written = header.size() + payload.size(),
        .checksum = crc
    };
}

PersistenceResult IndexPersistence::verify_integrity(const std::string& filepath) {
    std::ifstream in(filepath, std::ios::binary | std::ios::ate);
    if (!in) {
        return PersistenceResult{
            .success = false,
            .error_message = "File does not exist or cannot be opened: " + filepath
        };
    }

    std::streamsize file_size = in.tellg();
    if (file_size < 20) { // Header minimum size
        return PersistenceResult{
            .success = false,
            .error_message = "File is too small to contain a valid index header"
        };
    }

    in.seekg(0, std::ios::beg);
    std::vector<uint8_t> header_bytes(20);
    in.read(reinterpret_cast<char*>(header_bytes.data()), 20);

    const uint8_t* ptr = header_bytes.data();
    const uint8_t* end = ptr + 20;

    uint32_t magic = 0, version = 0, stored_crc = 0;
    uint64_t payload_size = 0;

    read_u32(ptr, end, magic);
    read_u32(ptr, end, version);
    read_u32(ptr, end, stored_crc);
    read_u64(ptr, end, payload_size);

    if (magic != MAGIC_HEADER) {
        return PersistenceResult{
            .success = false,
            .error_message = "Invalid magic bytes in index file header"
        };
    }

    if (version != FORMAT_VERSION) {
        return PersistenceResult{
            .success = false,
            .error_message = "Unsupported index format version: " + std::to_string(version)
        };
    }

    if (static_cast<uint64_t>(file_size - 20) != payload_size) {
        return PersistenceResult{
            .success = false,
            .error_message = "Index file size does not match declared payload length (file truncated or corrupted)"
        };
    }

    std::vector<uint8_t> payload(payload_size);
    in.read(reinterpret_cast<char*>(payload.data()), payload_size);

    uint32_t computed_crc = calculate_crc32(payload.data(), payload.size());
    if (computed_crc != stored_crc) {
        return PersistenceResult{
            .success = false,
            .error_message = "CRC32 checksum mismatch: expected " + std::to_string(stored_crc) +
                             ", computed " + std::to_string(computed_crc) + " (corrupted index file)"
        };
    }

    return PersistenceResult{
        .success = true,
        .error_message = "",
        .bytes_written = static_cast<size_t>(file_size),
        .checksum = computed_crc
    };
}

PersistenceResult IndexPersistence::load_from_file(InvertedIndex& index, const std::string& filepath) {
    auto verify = verify_integrity(filepath);
    if (!verify.success) {
        return verify;
    }

    std::ifstream in(filepath, std::ios::binary);
    in.seekg(20); // skip header

    std::vector<uint8_t> payload(verify.bytes_written - 20);
    in.read(reinterpret_cast<char*>(payload.data()), payload.size());

    const uint8_t* ptr = payload.data();
    const uint8_t* end = ptr + payload.size();

    uint64_t total_tokens = 0;
    if (!read_u64(ptr, end, total_tokens)) {
        return PersistenceResult{.success = false, .error_message = "Failed to parse total tokens"};
    }

    // Read docs
    uint32_t doc_count = 0;
    if (!read_u32(ptr, end, doc_count)) {
        return PersistenceResult{.success = false, .error_message = "Failed to parse doc count"};
    }

    std::unordered_map<DocId, Document> docs;
    docs.reserve(doc_count);
    for (uint32_t i = 0; i < doc_count; ++i) {
        Document d;
        if (!read_u32(ptr, end, d.id)) return PersistenceResult{.success = false, .error_message = "Failed to read doc id"};
        if (!read_string(ptr, end, d.title)) return PersistenceResult{.success = false, .error_message = "Failed to read title"};
        if (!read_string(ptr, end, d.source)) return PersistenceResult{.success = false, .error_message = "Failed to read source"};
        if (!read_string(ptr, end, d.date)) return PersistenceResult{.success = false, .error_message = "Failed to read date"};
        if (!read_string(ptr, end, d.doc_type)) return PersistenceResult{.success = false, .error_message = "Failed to read doc_type"};
        if (!read_string(ptr, end, d.content)) return PersistenceResult{.success = false, .error_message = "Failed to read content"};
        if (!read_u32(ptr, end, d.length)) return PersistenceResult{.success = false, .error_message = "Failed to read length"};
        if (!read_u32(ptr, end, d.unique_terms)) return PersistenceResult{.success = false, .error_message = "Failed to read unique terms"};
        docs[d.id] = std::move(d);
    }

    // Read deleted doc IDs
    uint32_t del_count = 0;
    if (!read_u32(ptr, end, del_count)) {
        return PersistenceResult{.success = false, .error_message = "Failed to parse deleted count"};
    }
    std::unordered_set<DocId> deleted;
    deleted.reserve(del_count);
    for (uint32_t i = 0; i < del_count; ++i) {
        DocId id = 0;
        if (!read_u32(ptr, end, id)) return PersistenceResult{.success = false, .error_message = "Failed to read deleted id"};
        deleted.insert(id);
    }

    // Read terms and compressed postings
    uint32_t term_count = 0;
    if (!read_u32(ptr, end, term_count)) {
        return PersistenceResult{.success = false, .error_message = "Failed to parse term count"};
    }
    std::unordered_map<std::string, PostingList> raw_idx;
    raw_idx.reserve(term_count);

    for (uint32_t i = 0; i < term_count; ++i) {
        std::string term;
        if (!read_string(ptr, end, term)) return PersistenceResult{.success = false, .error_message = "Failed to read term"};
        uint32_t compressed_len = 0;
        if (!read_u32(ptr, end, compressed_len)) return PersistenceResult{.success = false, .error_message = "Failed to read compressed len"};
        if (ptr + compressed_len > end) return PersistenceResult{.success = false, .error_message = "Compressed posting buffer overflow"};
        
        PostingList postings = PostingCompressor::decompress(std::span<const uint8_t>(ptr, compressed_len));
        ptr += compressed_len;
        raw_idx[std::move(term)] = std::move(postings);
    }

    index.restore_state(std::move(raw_idx), std::move(docs), std::move(deleted), static_cast<size_t>(total_tokens));
    return verify;
}

} // namespace indexforge
