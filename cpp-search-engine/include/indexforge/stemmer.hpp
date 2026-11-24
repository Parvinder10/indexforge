#pragma once

#include <string>
#include <string_view>

namespace indexforge {

class PorterStemmer {
public:
    static std::string stem(std::string_view word);

private:
    static bool is_consonant(const std::string& str, size_t i);
    static int measure(const std::string& str);
    static bool contains_vowel(const std::string& str);
    static bool double_consonant(const std::string& str, size_t i);
    static bool cvc(const std::string& str, size_t i);
    
    static void step1a(std::string& str);
    static void step1b(std::string& str);
    static void step1c(std::string& str);
    static void step2(std::string& str);
    static void step3(std::string& str);
    static void step4(std::string& str);
    static void step5a(std::string& str);
    static void step5b(std::string& str);

    static bool ends_with(const std::string& str, const std::string& suffix);
    static bool replace_suffix(std::string& str, const std::string& suffix, const std::string& replacement, int min_m = -1);
};

} // namespace indexforge
