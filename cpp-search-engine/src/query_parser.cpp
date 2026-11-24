#include "indexforge/query_parser.hpp"
#include <cctype>

namespace indexforge {

QueryParser::QueryParser(Tokenizer tokenizer)
    : tokenizer_(std::move(tokenizer)) {}

std::vector<QueryParser::QueryToken> QueryParser::tokenize_query(std::string_view query_str) const {
    std::vector<QueryToken> tokens;
    const size_t len = query_str.size();
    size_t i = 0;

    while (i < len) {
        while (i < len && std::isspace(static_cast<unsigned char>(query_str[i]))) {
            ++i;
        }
        if (i >= len) break;

        char c = query_str[i];

        if (c == '(') {
            tokens.push_back({TokenType::LPAREN, "("});
            ++i;
            continue;
        }
        if (c == ')') {
            tokens.push_back({TokenType::RPAREN, ")"});
            ++i;
            continue;
        }

        // Quoted phrase: "..."
        if (c == '"' || c == '\'') {
            char quote_char = c;
            ++i;
            size_t start = i;
            while (i < len && query_str[i] != quote_char) {
                ++i;
            }
            std::string phrase(query_str.substr(start, i - start));
            if (i < len && query_str[i] == quote_char) {
                ++i;
            }
            if (!phrase.empty()) {
                tokens.push_back({TokenType::PHRASE, std::move(phrase)});
            }
            continue;
        }

        // Unary NOT or minus prefix
        if (c == '-' || c == '!') {
            // If immediately followed by a character (like -term or !term)
            if (i + 1 < len && !std::isspace(static_cast<unsigned char>(query_str[i + 1]))) {
                tokens.push_back({TokenType::NOT, "NOT"});
                ++i;
                continue;
            }
        }

        // Read word
        size_t start = i;
        while (i < len && !std::isspace(static_cast<unsigned char>(query_str[i])) &&
               query_str[i] != '(' && query_str[i] != ')' && query_str[i] != '"' && query_str[i] != '\'') {
            ++i;
        }

        std::string raw_word(query_str.substr(start, i - start));
        // Check operators
        if (raw_word == "AND" || raw_word == "&&" || raw_word == "+") {
            tokens.push_back({TokenType::AND, "AND"});
        } else if (raw_word == "OR" || raw_word == "||") {
            tokens.push_back({TokenType::OR, "OR"});
        } else if (raw_word == "NOT") {
            tokens.push_back({TokenType::NOT, "NOT"});
        } else {
            tokens.push_back({TokenType::TERM, std::move(raw_word)});
        }
    }

    tokens.push_back({TokenType::END, ""});
    return tokens;
}

std::shared_ptr<QueryNode> QueryParser::parse_or(
    const std::vector<QueryToken>& tokens, size_t& pos) const {
    auto left = parse_and(tokens, pos);
    if (!left) return nullptr;

    std::vector<std::shared_ptr<QueryNode>> or_children;
    or_children.push_back(left);

    while (tokens[pos].type == TokenType::OR) {
        ++pos; // consume OR
        auto right = parse_and(tokens, pos);
        if (right) {
            or_children.push_back(right);
        }
    }

    if (or_children.size() == 1) {
        return or_children[0];
    }

    auto or_node = std::make_shared<OrNode>();
    or_node->children = std::move(or_children);
    return or_node;
}

std::shared_ptr<QueryNode> QueryParser::parse_and(
    const std::vector<QueryToken>& tokens, size_t& pos) const {
    auto left = parse_unary(tokens, pos);
    if (!left) return nullptr;

    std::vector<std::shared_ptr<QueryNode>> and_children;
    and_children.push_back(left);

    while (pos < tokens.size()) {
        if (tokens[pos].type == TokenType::AND) {
            ++pos; // consume AND
            auto right = parse_unary(tokens, pos);
            if (right) {
                and_children.push_back(right);
            }
        } else if (tokens[pos].type == TokenType::TERM ||
                   tokens[pos].type == TokenType::PHRASE ||
                   tokens[pos].type == TokenType::NOT ||
                   tokens[pos].type == TokenType::LPAREN) {
            // Implicit AND / juxtaposition
            auto right = parse_unary(tokens, pos);
            if (right) {
                and_children.push_back(right);
            }
        } else {
            break;
        }
    }

    if (and_children.size() == 1) {
        return and_children[0];
    }

    auto and_node = std::make_shared<AndNode>();
    and_node->children = std::move(and_children);
    return and_node;
}

std::shared_ptr<QueryNode> QueryParser::parse_unary(
    const std::vector<QueryToken>& tokens, size_t& pos) const {
    if (tokens[pos].type == TokenType::NOT) {
        ++pos; // consume NOT
        auto child = parse_unary(tokens, pos);
        if (child) {
            return std::make_shared<NotNode>(child);
        }
        return nullptr;
    }
    return parse_primary(tokens, pos);
}

std::shared_ptr<QueryNode> QueryParser::parse_primary(
    const std::vector<QueryToken>& tokens, size_t& pos) const {
    if (tokens[pos].type == TokenType::LPAREN) {
        ++pos; // consume '('
        auto expr = parse_or(tokens, pos);
        if (tokens[pos].type == TokenType::RPAREN) {
            ++pos; // consume ')'
        }
        return expr;
    }

    if (tokens[pos].type == TokenType::PHRASE) {
        std::string raw_phrase = tokens[pos].value;
        ++pos;
        
        // Tokenize phrase terms
        auto phrase_tokens = tokenizer_.tokenize(raw_phrase);
        std::vector<std::string> terms;
        for (const auto& pt : phrase_tokens) {
            if (!pt.text.empty()) {
                terms.push_back(pt.text);
            }
        }
        if (terms.empty()) return nullptr;
        if (terms.size() == 1) {
            return std::make_shared<TermNode>(terms[0]);
        }
        return std::make_shared<PhraseNode>(std::move(terms));
    }

    if (tokens[pos].type == TokenType::TERM) {
        std::string raw_term = tokens[pos].value;
        ++pos;
        std::string processed = tokenizer_.process_term(raw_term);
        if (processed.empty()) {
            return nullptr;
        }
        return std::make_shared<TermNode>(std::move(processed));
    }

    return nullptr;
}

void QueryParser::extract_terms(
    const QueryNode* node,
    std::vector<std::string>& terms,
    std::vector<std::vector<std::string>>& phrases,
    bool is_negated) const {
    if (!node) return;

    switch (node->type()) {
        case QueryNodeType::TERM: {
            if (!is_negated) {
                const auto* tn = static_cast<const TermNode*>(node);
                terms.push_back(tn->term);
            }
            break;
        }
        case QueryNodeType::PHRASE: {
            if (!is_negated) {
                const auto* pn = static_cast<const PhraseNode*>(node);
                phrases.push_back(pn->terms);
                for (const auto& t : pn->terms) {
                    terms.push_back(t);
                }
            }
            break;
        }
        case QueryNodeType::AND: {
            const auto* an = static_cast<const AndNode*>(node);
            for (const auto& child : an->children) {
                extract_terms(child.get(), terms, phrases, is_negated);
            }
            break;
        }
        case QueryNodeType::OR: {
            const auto* on = static_cast<const OrNode*>(node);
            for (const auto& child : on->children) {
                extract_terms(child.get(), terms, phrases, is_negated);
            }
            break;
        }
        case QueryNodeType::NOT: {
            const auto* nn = static_cast<const NotNode*>(node);
            extract_terms(nn->child.get(), terms, phrases, true);
            break;
        }
    }
}

ParsedQuery QueryParser::parse(std::string_view query_str) const {
    auto tokens = tokenize_query(query_str);
    size_t pos = 0;
    auto root = parse_or(tokens, pos);

    ParsedQuery result;
    result.root = root;
    if (root) {
        extract_terms(root.get(), result.positive_terms, result.phrase_terms);
    }
    return result;
}

} // namespace indexforge
