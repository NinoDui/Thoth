#pragma once

#include <string>
#include <vector>

enum class TokenLabel { Correct, Missing, Extra, Different };

struct TokenResult {
    std::string token;
    TokenLabel label;
};

struct ScoringResult {
    double score = 0.0;
    std::vector<TokenResult> alignedTokens;  // one per reference token
    std::vector<std::string> extraTokens;    // hypothesis tokens not matched to reference
};

class ISentenceScorer {
   public:
    virtual ~ISentenceScorer() = default;

    virtual double score(const std::string& reference, const std::string& hypothesis) const = 0;

    virtual ScoringResult scoreDetail(const std::string& reference,
                                      const std::string& hypothesis) const = 0;

    virtual std::string name() const = 0;
};
