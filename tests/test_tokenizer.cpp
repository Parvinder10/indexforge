#include "test_framework.hpp"
#include "indexforge/tokenizer.hpp"
#include "indexforge/stop_words.hpp"

using namespace indexforge;

TEST_CASE("Tokenizer - Basic tokenization and case normalization") {
    Tokenizer tokenizer;
    auto tokens = tokenizer.tokenize("Public Tender for Information Technology");
    
    // "for" is a stop word and filtered out
    ASSERT_GT(tokens.size(), 0);
    // Tokens should be lowercased and stemmed
    for (const auto& tok : tokens) {
        ASSERT_TRUE(!tok.text.empty());
        ASSERT_GE(tok.end_offset, tok.start_offset);
    }
}

TEST_CASE("Tokenizer - Stop-word removal") {
    TokenizerOptions opts{.enable_stemming = false, .enable_stop_words = true};
    Tokenizer tokenizer(opts);
    
    auto tokens = tokenizer.tokenize("this is a test of the search engine");
    // "this", "is", "a", "of", "the" are stop words
    // Only "test", "search", "engine" should remain
    ASSERT_EQ(tokens.size(), 3);
    ASSERT_EQ(tokens[0].text, "test");
    ASSERT_EQ(tokens[1].text, "search");
    ASSERT_EQ(tokens[2].text, "engine");
}

TEST_CASE("Tokenizer - Position tracking preserves phrase distances") {
    TokenizerOptions opts{.enable_stemming = false, .enable_stop_words = true};
    Tokenizer tokenizer(opts);
    
    // "cloud in the computing"
    // "cloud" at pos 0, "in" at pos 1 (filtered), "the" at pos 2 (filtered), "computing" at pos 3
    auto tokens = tokenizer.tokenize("cloud in the computing");
    ASSERT_EQ(tokens.size(), 2);
    ASSERT_EQ(tokens[0].position, 0);
    ASSERT_EQ(tokens[1].position, 3);
}

TEST_CASE("Tokenizer - Invalid and boundary inputs") {
    Tokenizer tokenizer;
    
    // Empty text
    auto empty_tokens = tokenizer.tokenize("");
    ASSERT_TRUE(empty_tokens.empty());

    // Punctuation and symbols only
    auto symbol_tokens = tokenizer.tokenize("!@#$%^&*()_+{}[]:;\"'<>?,./~`");
    ASSERT_TRUE(symbol_tokens.empty());

    // Extremely long string
    std::string long_str(10000, 'a');
    auto long_tokens = tokenizer.tokenize(long_str);
    // Word length exceeds max_token_len (64)
    ASSERT_TRUE(long_tokens.empty());
}
