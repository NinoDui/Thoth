#include "internal/Segmenter.h"

Sentences RegexSegmenter::segment(std::string_view text) const {
    Sentences sentences;
    if (text.empty()) return sentences;

    std::regex re(R"([.?!\n]\s+|$)");

    std::string content(text);
    std::sregex_token_iterator first{content.begin(), content.end(), re, -1};
    std::sregex_token_iterator last;  // none by default

    for (auto it = first; it != last; ++it) {
        if (it->length() > 0 && isValidSentence(it->str())) {
            sentences.push_back(it->str());
        }
    }

    return sentences;
}

bool RegexSegmenter::isValidSentence(const std::string& raw) const {
    // Case 1, all numbers -> False
    bool isAllNumbers = true;
    for (auto& c : raw) {
        if (!std::isdigit(c)) {
            isAllNumbers = false;
            break;
        }
    }
    if (isAllNumbers) return false;

    // Case 2, all .!?, -> False
    bool isAllPunctuation = true;
    for (auto& c : raw) {
        if (!std::ispunct(c)) {
            isAllPunctuation = false;
            break;
        }
    }
    if (isAllPunctuation) return false;

    return true;
}

std::unique_ptr<Segmenter> createRegexSegmenter() { return std::make_unique<RegexSegmenter>(); }