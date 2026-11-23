#pragma once

#include "indexforge/types.hpp"
#include "indexforge/tokenizer.hpp"
#include <string>
#include <vector>
#include <memory>
#include <string_view>

namespace indexforge {

enum class QueryNodeType {
    TERM,
    PHRASE,
    AND,
    OR,
    NOT
};

struct QueryNode {
    virtual ~QueryNode() = default;
    virtual QueryNodeType type() const = 0;
};

struct TermNode : public QueryNode {
    std::string term;
    explicit TermNode(std::string t) : term(std::move(t)) {}
    QueryNodeType type() const override { return QueryNodeType::TERM; }
};

struct PhraseNode : public QueryNode {
    std::vector<std::string> terms;
    explicit PhraseNode(std::vector<std::string> t) : terms(std::move(t)) {}
    QueryNodeType type() const override { return QueryNodeType::PHRASE; }
};

struct AndNode : public QueryNode {
    std::vector<std::shared_ptr<QueryNode>> children;
    QueryNodeType type() const override { return QueryNodeType::AND; }
};

struct OrNode : public QueryNode {
    std::vector<std::shared_ptr<QueryNode>> children;
    QueryNodeType type() const override { return QueryNodeType::OR; }
};

struct NotNode : public QueryNode {
    std::shared_ptr<QueryNode> child;
    explicit NotNode(std::shared_ptr<QueryNode> c) : child(std::move(c)) {}
    QueryNodeType type() const override { return QueryNodeType::NOT; }
};

struct ParsedQuery {
    std::shared_ptr<QueryNode> root;
    std::vector<std::string> positive_terms; // All positive terms for snippet & scoring
    std::vector<std::vector<std::string>> phrase_terms; // Phrases
};

class QueryParser {
public:
    explicit QueryParser(Tokenizer tokenizer = Tokenizer{});

    ParsedQuery parse(std::string_view query_str) const;

private:
    Tokenizer tokenizer_;

    enum class TokenType {
        TERM,
        PHRASE,
        AND,
        OR,
        NOT,
        LPAREN,
        RPAREN,
        END
    };

    struct QueryToken {
        TokenType type;
        std::string value;
    };

    std::vector<QueryToken> tokenize_query(std::string_view query_str) const;

    std::shared_ptr<QueryNode> parse_or(
        const std::vector<QueryToken>& tokens, size_t& pos) const;
    std::shared_ptr<QueryNode> parse_and(
        const std::vector<QueryToken>& tokens, size_t& pos) const;
    std::shared_ptr<QueryNode> parse_unary(
        const std::vector<QueryToken>& tokens, size_t& pos) const;
    std::shared_ptr<QueryNode> parse_primary(
        const std::vector<QueryToken>& tokens, size_t& pos) const;

    void extract_terms(
        const QueryNode* node,
        std::vector<std::string>& terms,
        std::vector<std::vector<std::string>>& phrases,
        bool is_negated = false) const;
};

} // namespace indexforge
