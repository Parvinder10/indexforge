#pragma once

#include "indexforge/tokenizer.hpp"
#include <string>
#include <vector>
#include <string_view>

namespace indexforge {

class SnippetGenerator {
public:
    explicit SnippetGenerator(Tokenizer tokenizer = Tokenizer{});

    // Generate highlighted text snippet from raw document content
    std::string generate_snippet(
        std::string_view content,
        const std::vector<std::string>& query_terms,
        size_t max_chars = 220,
        std::string_view pre_tag = "<mark>",
        std::string_view post_tag = "</mark>") const;

private:
    Tokenizer tokenizer_;
};

} // namespace indexforge
