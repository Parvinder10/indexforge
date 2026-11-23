#pragma once

#include "indexforge/types.hpp"
#include <string>

namespace indexforge {

struct Document {
    DocId id{0};
    std::string title;
    std::string source;
    std::string date;
    std::string doc_type;
    std::string content;
    uint32_t length{0};
    uint32_t unique_terms{0};
};

struct DocumentMeta {
    DocId id{0};
    std::string title;
    std::string source;
    std::string date;
    std::string doc_type;
    uint32_t length{0};
};

} // namespace indexforge
