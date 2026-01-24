#include "Segmenter.h"


Sentences RegexSegmenter::segment(std::string_view text) const {
    Sentences sentences;
    if (text.empty()) return sentences;

    std::regex re(R"([.?!]\s+|$)");

    std::string content(text);
    std::sregex_token_iterator first{content.begin(), content.end(), re, -1};
    std::sregex_token_iterator last;

    for (auto it = first; it != last; ++it) {
        if (it->length() > 0) {
            sentences.push_back(it->str());
        }
    }

    return sentences;
}


std::unique_ptr<Segmenter> createRegexSegmenter() {
    return std::make_unique<RegexSegmenter>();
}