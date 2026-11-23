#pragma once

#include "indexforge/types.hpp"
#include <string>
#include <vector>
#include <string_view>

namespace indexforge {

struct TokenizerOptions {
    bool enable_stemming{true};
    bool enable_stop_words{true};
    size_t min_token_len{2};
    size_t max_token_len{64};
};

class Tokenizer {
public:
    explicit Tokenizer(TokenizerOptions options = TokenizerOptions{});

    // Tokenize full text into processed tokens with positions and offsets
    std::vector<Token> tokenize(std::string_view text) const;

    // Tokenize text for query processing (can preserve stop words or stemming)
    std::vector<std::string> tokenize_query_terms(std::string_view query) const;

    // Single term processing: lowercase, filter, stem
    std::string process_term(std::string_view raw_term) const;

    const TokenizerOptions& options() const { return options_; }
    void set_options(const TokenizerOptions& opts) { options_ = opts; }

private:
    TokenizerOptions options_;
};

} // namespace indexforge
