#pragma once

#include <string>
#include <vector>

#include "thoth/ISentenceScorer.h"

class WERScorer : public ISentenceScorer {
   public:
    double score(const std::string& reference, const std::string& hypothesis) const override;
    ScoringResult scoreDetail(const std::string& reference,
                              const std::string& hypothesis) const override;
    std::string name() const override;

   private:
    static std::string normalize(const std::string& text);
    static std::vector<std::string> tokenize(const std::string& text);
    static size_t editDistance(const std::vector<std::string>& ref,
                               const std::vector<std::string>& hyp);
    static ScoringResult alignTokens(const std::vector<std::string>& ref,
                                     const std::vector<std::string>& hyp);
};
