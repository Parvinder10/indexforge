#include "indexforge/stemmer.hpp"
#include <cctype>
#include <algorithm>

namespace indexforge {

bool PorterStemmer::is_consonant(const std::string& str, size_t i) {
    char c = str[i];
    switch (c) {
        case 'a': case 'e': case 'i': case 'o': case 'u':
            return false;
        case 'y':
            return (i == 0) ? true : !is_consonant(str, i - 1);
        default:
            return true;
    }
}

int PorterStemmer::measure(const std::string& str) {
    if (str.empty()) return 0;
    int m = 0;
    size_t i = 0;
    size_t n = str.size();
    
    // Skip initial consonants
    while (i < n && is_consonant(str, i)) ++i;
    
    while (i < n) {
        // Find sequence of vowels
        while (i < n && !is_consonant(str, i)) ++i;
        if (i >= n) break;
        // Find sequence of consonants
        while (i < n && is_consonant(str, i)) ++i;
        ++m;
    }
    return m;
}

bool PorterStemmer::contains_vowel(const std::string& str) {
    for (size_t i = 0; i < str.size(); ++i) {
        if (!is_consonant(str, i)) return true;
    }
    return false;
}

bool PorterStemmer::double_consonant(const std::string& str, size_t i) {
    if (i < 1 || i >= str.size()) return false;
    if (str[i] != str[i - 1]) return false;
    return is_consonant(str, i);
}

bool PorterStemmer::cvc(const std::string& str, size_t i) {
    if (i < 2 || i >= str.size()) return false;
    if (!is_consonant(str, i) || is_consonant(str, i - 1) || !is_consonant(str, i - 2)) return false;
    char ch = str[i];
    if (ch == 'w' || ch == 'x' || ch == 'y') return false;
    return true;
}

bool PorterStemmer::ends_with(const std::string& str, const std::string& suffix) {
    if (str.size() < suffix.size()) return false;
    return str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool PorterStemmer::replace_suffix(std::string& str, const std::string& suffix, const std::string& replacement, int min_m) {
    if (!ends_with(str, suffix)) return false;
    std::string stem_part = str.substr(0, str.size() - suffix.size());
    if (min_m >= 0 && measure(stem_part) <= min_m) return false;
    str = stem_part + replacement;
    return true;
}

void PorterStemmer::step1a(std::string& str) {
    if (ends_with(str, "sses")) {
        str = str.substr(0, str.size() - 2);
    } else if (ends_with(str, "ies")) {
        str = str.substr(0, str.size() - 2);
    } else if (ends_with(str, "ss")) {
        // keep ss
    } else if (ends_with(str, "s")) {
        str = str.substr(0, str.size() - 1);
    }
}

void PorterStemmer::step1b(std::string& str) {
    bool second_third = false;
    if (ends_with(str, "eed")) {
        std::string stem_part = str.substr(0, str.size() - 3);
        if (measure(stem_part) > 0) {
            str = stem_part + "ee";
        }
    } else if (ends_with(str, "ed")) {
        std::string stem_part = str.substr(0, str.size() - 2);
        if (contains_vowel(stem_part)) {
            str = stem_part;
            second_third = true;
        }
    } else if (ends_with(str, "ing")) {
        std::string stem_part = str.substr(0, str.size() - 3);
        if (contains_vowel(stem_part)) {
            str = stem_part;
            second_third = true;
        }
    }

    if (second_third) {
        if (ends_with(str, "at") || ends_with(str, "bl") || ends_with(str, "iz")) {
            str += "e";
        } else if (double_consonant(str, str.size() - 1) && !ends_with(str, "l") && !ends_with(str, "s") && !ends_with(str, "z")) {
            str.pop_back();
        } else if (measure(str) == 1 && cvc(str, str.size() - 1)) {
            str += "e";
        }
    }
}

void PorterStemmer::step1c(std::string& str) {
    if (ends_with(str, "y")) {
        std::string stem_part = str.substr(0, str.size() - 1);
        if (contains_vowel(stem_part)) {
            str = stem_part + "i";
        }
    }
}

void PorterStemmer::step2(std::string& str) {
    if (ends_with(str, "ational")) replace_suffix(str, "ational", "ate", 0);
    else if (ends_with(str, "tional")) replace_suffix(str, "tional", "tion", 0);
    else if (ends_with(str, "enci")) replace_suffix(str, "enci", "ence", 0);
    else if (ends_with(str, "anci")) replace_suffix(str, "anci", "ance", 0);
    else if (ends_with(str, "izer")) replace_suffix(str, "izer", "ize", 0);
    else if (ends_with(str, "abli")) replace_suffix(str, "abli", "able", 0);
    else if (ends_with(str, "alli")) replace_suffix(str, "alli", "al", 0);
    else if (ends_with(str, "entli")) replace_suffix(str, "entli", "ent", 0);
    else if (ends_with(str, "eli")) replace_suffix(str, "eli", "e", 0);
    else if (ends_with(str, "ousli")) replace_suffix(str, "ousli", "ous", 0);
    else if (ends_with(str, "ization")) replace_suffix(str, "ization", "ize", 0);
    else if (ends_with(str, "ation")) replace_suffix(str, "ation", "ate", 0);
    else if (ends_with(str, "ator")) replace_suffix(str, "ator", "ate", 0);
    else if (ends_with(str, "alism")) replace_suffix(str, "alism", "al", 0);
    else if (ends_with(str, "iveness")) replace_suffix(str, "iveness", "ive", 0);
    else if (ends_with(str, "fulness")) replace_suffix(str, "fulness", "ful", 0);
    else if (ends_with(str, "ousness")) replace_suffix(str, "ousness", "ous", 0);
    else if (ends_with(str, "aliti")) replace_suffix(str, "aliti", "al", 0);
    else if (ends_with(str, "iviti")) replace_suffix(str, "iviti", "ive", 0);
    else if (ends_with(str, "biliti")) replace_suffix(str, "biliti", "ble", 0);
}

void PorterStemmer::step3(std::string& str) {
    if (ends_with(str, "icate")) replace_suffix(str, "icate", "ic", 0);
    else if (ends_with(str, "ative")) replace_suffix(str, "ative", "", 0);
    else if (ends_with(str, "alize")) replace_suffix(str, "alize", "al", 0);
    else if (ends_with(str, "iciti")) replace_suffix(str, "iciti", "ic", 0);
    else if (ends_with(str, "ical")) replace_suffix(str, "ical", "ic", 0);
    else if (ends_with(str, "ful")) replace_suffix(str, "ful", "", 0);
    else if (ends_with(str, "ness")) replace_suffix(str, "ness", "", 0);
}

void PorterStemmer::step4(std::string& str) {
    if (ends_with(str, "al")) replace_suffix(str, "al", "", 1);
    else if (ends_with(str, "ance")) replace_suffix(str, "ance", "", 1);
    else if (ends_with(str, "ence")) replace_suffix(str, "ence", "", 1);
    else if (ends_with(str, "er")) replace_suffix(str, "er", "", 1);
    else if (ends_with(str, "ic")) replace_suffix(str, "ic", "", 1);
    else if (ends_with(str, "able")) replace_suffix(str, "able", "", 1);
    else if (ends_with(str, "ible")) replace_suffix(str, "ible", "", 1);
    else if (ends_with(str, "ant")) replace_suffix(str, "ant", "", 1);
    else if (ends_with(str, "ement")) replace_suffix(str, "ement", "", 1);
    else if (ends_with(str, "ment")) replace_suffix(str, "ment", "", 1);
    else if (ends_with(str, "ent")) replace_suffix(str, "ent", "", 1);
    else if (ends_with(str, "ion")) {
        std::string stem_part = str.substr(0, str.size() - 3);
        if (!stem_part.empty() && (stem_part.back() == 's' || stem_part.back() == 't') && measure(stem_part) > 1) {
            str = stem_part;
        }
    }
    else if (ends_with(str, "ou")) replace_suffix(str, "ou", "", 1);
    else if (ends_with(str, "ism")) replace_suffix(str, "ism", "", 1);
    else if (ends_with(str, "ate")) replace_suffix(str, "ate", "", 1);
    else if (ends_with(str, "iti")) replace_suffix(str, "iti", "", 1);
    else if (ends_with(str, "ous")) replace_suffix(str, "ous", "", 1);
    else if (ends_with(str, "ive")) replace_suffix(str, "ive", "", 1);
    else if (ends_with(str, "ize")) replace_suffix(str, "ize", "", 1);
}

void PorterStemmer::step5a(std::string& str) {
    if (ends_with(str, "e")) {
        std::string stem_part = str.substr(0, str.size() - 1);
        int m = measure(stem_part);
        if (m > 1 || (m == 1 && !cvc(stem_part, stem_part.size() - 1))) {
            str = stem_part;
        }
    }
}

void PorterStemmer::step5b(std::string& str) {
    if (measure(str) > 1 && double_consonant(str, str.size() - 1) && ends_with(str, "l")) {
        str.pop_back();
    }
}

std::string PorterStemmer::stem(std::string_view word) {
    if (word.size() <= 2) {
        std::string res;
        for (char c : word) res.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        return res;
    }

    std::string str;
    str.reserve(word.size());
    for (char c : word) {
        str.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }

    step1a(str);
    step1b(str);
    step1c(str);
    step2(str);
    step3(str);
    step4(str);
    step5a(str);
    step5b(str);

    return str;
}

} // namespace indexforge
