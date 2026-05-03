#pragma once

#include <memory>
#include <string>
#include <vector>

using Sentences = std::vector<std::string>;

class Segmenter {
   public:
    virtual ~Segmenter() = default;

    [[nodiscard]] virtual Sentences segment(std::string_view text) const = 0;
};

std::unique_ptr<Segmenter> createRegexSegmenter();

class RegexSegmenter : public Segmenter {
   public:
    ~RegexSegmenter() override = default;

    Sentences segment(std::string_view text) const override;

   private:
    bool isValidSentence(const std::string& raw) const;
};
