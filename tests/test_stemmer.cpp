#include "test_framework.hpp"
#include "indexforge/stemmer.hpp"

using namespace indexforge;

TEST_CASE("Porter Stemmer - Common English verb & noun transformations") {
    ASSERT_EQ(PorterStemmer::stem("connects"), "connect");
    ASSERT_EQ(PorterStemmer::stem("connecting"), "connect");
    ASSERT_EQ(PorterStemmer::stem("connection"), "connect");
    ASSERT_EQ(PorterStemmer::stem("connections"), "connect");

    ASSERT_EQ(PorterStemmer::stem("regulations"), "regul");
    ASSERT_EQ(PorterStemmer::stem("regulatory"), "regulatori");

    ASSERT_EQ(PorterStemmer::stem("procurement"), "procur");
    ASSERT_EQ(PorterStemmer::stem("procured"), "procur");

    ASSERT_EQ(PorterStemmer::stem("policies"), "polici");
}

TEST_CASE("Porter Stemmer - Short words and edges") {
    // 2-letter or shorter words are unchanged
    ASSERT_EQ(PorterStemmer::stem("a"), "a");
    ASSERT_EQ(PorterStemmer::stem("it"), "it");
    ASSERT_EQ(PorterStemmer::stem(""), "");
}
