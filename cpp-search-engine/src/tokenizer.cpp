#include "indexforge/tokenizer.hpp"
#include "indexforge/stemmer.hpp"
#include "indexforge/stop_words.hpp"
#include <cctype>

namespace indexforge {

Tokenizer::Tokenizer(TokenizerOptions options)
    : options_(options) {}

std::string Tokenizer::process_term(std::string_view raw_term) const {
    if (raw_term.size() < options_.min_token_len || raw_term.size() > options_.max_token_len) {
        return "";
    }

    std::string lower;
    lower.reserve(raw_term.size());
    for (char c : raw_term) {
        lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }

    if (options_.enable_stop_words && StopWords::is_stop_word(lower)) {
        return "";
    }

    if (options_.enable_stemming) {
        return PorterStemmer::stem(lower);
    }

    return lower;
}

std::vector<Token> Tokenizer::tokenize(std::string_view text) const {
    std::vector<Token> tokens;
    const size_t len = text.size();
    size_t i = 0;
    Position current_position = 0;

    while (i < len) {
        // Skip non-alphanumeric characters
        while (i < len && !std::isalnum(static_cast<unsigned char>(text[i]))) {
            ++i;
        }

        if (i >= len) break;

        size_t start_offset = i;
        while (i < len && (std::isalnum(static_cast<unsigned char>(text[i])) || text[i] == '-' || text[i] == '_')) {
            // If hyphen/underscore is at the end of word, break
            if ((text[i] == '-' || text[i] == '_') && (i + 1 >= len || !std::isalnum(static_cast<unsigned char>(text[i + 1])))) {
                break;
            }
            ++i;
        }
        size_t end_offset = i;

        std::string_view raw_span = text.substr(start_offset, end_offset - start_offset);
        
        // Trim trailing hyphens if any
        while (!raw_span.empty() && (raw_span.back() == '-' || raw_span.back() == '_')) {
            raw_span.remove_suffix(1);
            end_offset--;
        }

        if (raw_span.size() >= options_.min_token_len && raw_span.size() <= options_.max_token_len) {
            std::string lower;
            lower.reserve(raw_span.size());
            for (char c : raw_span) {
                lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
            }

            bool is_stop = options_.enable_stop_words && StopWords::is_stop_word(lower);
            if (!is_stop) {
                std::string processed = options_.enable_stemming ? PorterStemmer::stem(lower) : lower;
                if (!processed.empty()) {
                    tokens.push_back(Token{
                        .text = std::move(processed),
                        .position = current_position,
                        .start_offset = start_offset,
                        .end_offset = end_offset
                    });
                }
            }
            // Always increment position to preserve phrase distance even over stop words!
            ++current_position;
        }
    }

    return tokens;
}

std::vector<std::string> Tokenizer::tokenize_query_terms(std::string_view query) const {
    std::vector<std::string> terms;
    const size_t len = query.size();
    size_t i = 0;

    while (i < len) {
        while (i < len && !std::isalnum(static_cast<unsigned char>(query[i]))) {
            ++i;
        }
        if (i >= len) break;

        size_t start = i;
        while (i < len && (std::isalnum(static_cast<unsigned char>(query[i])) || query[i] == '-' || query[i] == '_')) {
            if ((query[i] == '-' || query[i] == '_') && (i + 1 >= len || !std::isalnum(static_cast<unsigned char>(query[i + 1])))) {
                break;
            }
            ++i;
        }
        size_t end = i;
        std::string_view raw = query.substr(start, end - start);
        std::string processed = process_term(raw);
        if (!processed.empty()) {
            terms.push_back(std::move(processed));
        }
    }

    return terms;
}

} // namespace indexforge
