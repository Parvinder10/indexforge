#pragma once

#include "indexforge/types.hpp"
#include "indexforge/inverted_index.hpp"
#include <string>
#include <string_view>

namespace indexforge {

struct PersistenceResult {
    bool success{false};
    std::string error_message;
    size_t bytes_written{0};
    uint32_t checksum{0};
};

class IndexPersistence {
public:
    static constexpr uint32_t MAGIC_HEADER = 0x49445846; // "IDXF"
    static constexpr uint32_t FORMAT_VERSION = 1;

    // Save inverted index to binary file on disk
    static PersistenceResult save_to_file(const InvertedIndex& index, const std::string& filepath);

    // Load inverted index from binary file on disk
    static PersistenceResult load_from_file(InvertedIndex& index, const std::string& filepath);

    // Verify binary index integrity without loading into memory
    static PersistenceResult verify_integrity(const std::string& filepath);

    // Calculate CRC32 checksum over raw buffer
    static uint32_t calculate_crc32(const uint8_t* data, size_t length);
};

} // namespace indexforge
