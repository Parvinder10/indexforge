#include "indexforge/snippet_generator.hpp"
#include <unordered_set>
#include <algorithm>
#include <cctype>

namespace indexforge {

SnippetGenerator::SnippetGenerator(Tokenizer tokenizer)
    : tokenizer_(std::move(tokenizer)) {}

std::string SnippetGenerator::generate_snippet(
    std::string_view content,
    const std::vector<std::string>& query_terms,
    size_t max_chars,
    std::string_view pre_tag,
    std::string_view post_tag) const {
    if (content.empty()) return "";

    std::unordered_set<std::string> query_term_set;
    for (const auto& t : query_terms) {
        query_term_set.insert(t);
        // Also stem if not already stemmed
        query_term_set.insert(tokenizer_.process_term(t));
    }

    // Tokenize full text preserving start and end byte offsets
    auto tokens = tokenizer_.tokenize(content);

    // Identify which tokens match query terms
    std::vector<size_t> match_indices;
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (query_term_set.find(tokens[i].text) != query_term_set.end()) {
            match_indices.push_back(i);
        }
    }

    size_t win_start_byte = 0;
    size_t win_end_byte = std::min(content.size(), max_chars);

    if (!match_indices.empty()) {
        // Sliding window over match_indices to find best window
        size_t best_score = 0;
        size_t best_token_start = 0;
        size_t best_token_end = 0;

        for (size_t i = 0; i < match_indices.size(); ++i) {
            size_t start_tok = match_indices[i];
            size_t start_byte = tokens[start_tok].start_offset;
            size_t end_byte_limit = start_byte + max_chars;

            std::unordered_set<std::string> seen_in_win;
            size_t count = 0;
            size_t end_tok = start_tok;

            for (size_t j = i; j < match_indices.size(); ++j) {
                size_t cur_tok = match_indices[j];
                if (tokens[cur_tok].end_offset > end_byte_limit) break;
                end_tok = cur_tok;
                seen_in_win.insert(tokens[cur_tok].text);
                count++;
            }

            // Score = unique terms * 10 + total term count
            size_t score = seen_in_win.size() * 10 + count;
            if (score > best_score) {
                best_score = score;
                best_token_start = start_tok;
                best_token_end = end_tok;
            }
        }

        // Center the window around [best_token_start, best_token_end]
        size_t range_start = tokens[best_token_start].start_offset;
        size_t range_end = tokens[best_token_end].end_offset;
        size_t match_len = range_end - range_start;

        if (match_len < max_chars) {
            size_t extra = max_chars - match_len;
            size_t lead = extra / 2;
            win_start_byte = (range_start >= lead) ? (range_start - lead) : 0;
            win_end_byte = std::min(content.size(), win_start_byte + max_chars);
        } else {
            win_start_byte = range_start;
            win_end_byte = std::min(content.size(), range_start + max_chars);
        }
    }

    // Snap to word boundaries
    while (win_start_byte > 0 && !std::isspace(static_cast<unsigned char>(content[win_start_byte]))) {
        --win_start_byte;
    }
    while (win_start_byte < content.size() && std::isspace(static_cast<unsigned char>(content[win_start_byte]))) {
        ++win_start_byte;
    }

    while (win_end_byte < content.size() && !std::isspace(static_cast<unsigned char>(content[win_end_byte]))) {
        ++win_end_byte;
    }

    // Build snippet string with highlights
    std::string snippet;
    bool prefix_ellipsis = (win_start_byte > 0);
    bool suffix_ellipsis = (win_end_byte < content.size());

    if (prefix_ellipsis) {
        snippet += "... ";
    }

    size_t cursor = win_start_byte;
    for (const auto& tok : tokens) {
        if (tok.end_offset <= win_start_byte) continue;
        if (tok.start_offset >= win_end_byte) break;

        if (tok.start_offset > cursor) {
            snippet.append(content.data() + cursor, tok.start_offset - cursor);
        }

        bool is_match = (query_term_set.find(tok.text) != query_term_set.end());
        if (is_match) {
            snippet.append(pre_tag);
            snippet.append(content.data() + tok.start_offset, tok.end_offset - tok.start_offset);
            snippet.append(post_tag);
        } else {
            snippet.append(content.data() + tok.start_offset, tok.end_offset - tok.start_offset);
        }
        cursor = tok.end_offset;
    }

    if (cursor < win_end_byte) {
        snippet.append(content.data() + cursor, win_end_byte - cursor);
    }

    if (suffix_ellipsis) {
        snippet += " ...";
    }

    return snippet;
}

} // namespace indexforge
