#pragma once

#include "indexforge/types.hpp"
#include <vector>
#include <cstdint>
#include <algorithm>

namespace indexforge {

struct Posting {
    DocId doc_id{0};
    uint32_t term_frequency{0};
    std::vector<Position> positions;

    bool operator<(const Posting& other) const {
        return doc_id < other.doc_id;
    }
};

using PostingList = std::vector<Posting>;

} // namespace indexforge
