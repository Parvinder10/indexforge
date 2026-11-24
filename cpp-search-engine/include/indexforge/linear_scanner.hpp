#pragma once

#include "indexforge/types.hpp"
#include "indexforge/document.hpp"
#include "indexforge/tokenizer.hpp"
#include <vector>
#include <string>

namespace indexforge {

class LinearScanner {
public:
    explicit LinearScanner(Tokenizer tokenizer = Tokenizer{});

    void add_document(Document doc);
    void clear();

    // Linearly scan every document, matching terms sequentially without an inverted index
    std::vector<SearchHit> search(
        const std::string& query,
        size_t limit = 20) const;

    size_t document_count() const { return documents_.size(); }

private:
    Tokenizer tokenizer_;
    std::vector<Document> documents_;
};

} // namespace indexforge
