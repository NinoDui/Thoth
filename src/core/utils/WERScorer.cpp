#include "thoth/WERScorer.h"

#include <algorithm>
#include <cctype>
#include <sstream>

std::string WERScorer::normalize(const std::string& text) {
    std::string result;
    result.reserve(text.size());
    for (unsigned char c : text) {
        if (std::isalpha(c) || std::isspace(c)) {
            result += static_cast<char>(std::tolower(c));
        } else if (std::ispunct(c)) {
            result += ' ';
        }
    }
    return result;
}

std::vector<std::string> WERScorer::tokenize(const std::string& text) {
    std::vector<std::string> tokens;
    std::istringstream iss(text);
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

size_t WERScorer::editDistance(const std::vector<std::string>& ref,
                               const std::vector<std::string>& hyp) {
    const size_t n = ref.size();
    const size_t m = hyp.size();
    std::vector<std::vector<size_t>> dp(n + 1, std::vector<size_t>(m + 1, 0));

    for (size_t i = 0; i <= n; ++i) dp[i][0] = i;
    for (size_t j = 0; j <= m; ++j) dp[0][j] = j;

    for (size_t i = 1; i <= n; ++i) {
        for (size_t j = 1; j <= m; ++j) {
            if (ref[i - 1] == hyp[j - 1]) {
                dp[i][j] = dp[i - 1][j - 1];
            } else {
                dp[i][j] = 1 + std::min({dp[i - 1][j], dp[i][j - 1], dp[i - 1][j - 1]});
            }
        }
    }
    return dp[n][m];
}

double WERScorer::score(const std::string& reference, const std::string& hypothesis) const {
    auto refTokens = tokenize(normalize(reference));
    auto hypTokens = tokenize(normalize(hypothesis));

    if (refTokens.empty()) {
        return hypTokens.empty() ? 1.0 : 0.0;
    }

    double wer = static_cast<double>(editDistance(refTokens, hypTokens)) /
                 static_cast<double>(refTokens.size());
    return std::max(0.0, 1.0 - std::min(1.0, wer));
}

ScoringResult WERScorer::scoreDetail(const std::string& reference,
                                     const std::string& hypothesis) const {
    auto refTokens = tokenize(normalize(reference));
    auto hypTokens = tokenize(normalize(hypothesis));

    if (refTokens.empty()) {
        ScoringResult result;
        result.score = hypTokens.empty() ? 1.0 : 0.0;
        return result;
    }

    return alignTokens(refTokens, hypTokens);
}

ScoringResult WERScorer::alignTokens(const std::vector<std::string>& ref,
                                     const std::vector<std::string>& hyp) {
    int n = static_cast<int>(ref.size());
    int m = static_cast<int>(hyp.size());

    std::vector<std::vector<int>> dp(n + 1, std::vector<int>(m + 1, 0));
    for (int i = 0; i <= n; ++i) dp[i][0] = i;
    for (int j = 0; j <= m; ++j) dp[0][j] = j;

    for (int i = 1; i <= n; ++i) {
        for (int j = 1; j <= m; ++j) {
            if (ref[i - 1] == hyp[j - 1]) {
                dp[i][j] = dp[i - 1][j - 1];
            } else {
                dp[i][j] = 1 + std::min({dp[i - 1][j], dp[i][j - 1], dp[i - 1][j - 1]});
            }
        }
    }

    ScoringResult result;
    double wer = static_cast<double>(dp[n][m]) / static_cast<double>(ref.size());
    result.score = std::max(0.0, 1.0 - std::min(1.0, wer));

    int i = n;
    int j = m;
    std::vector<TokenResult> reverse;
    std::vector<std::string> extraReverse;

    while (i > 0 || j > 0) {
        if (i > 0 && j > 0 && ref[i - 1] == hyp[j - 1]) {
            reverse.push_back({ref[i - 1], TokenLabel::Correct});
            --i;
            --j;
        } else if (i > 0 && j > 0 && dp[i][j] == dp[i - 1][j - 1] + 1) {
            reverse.push_back({ref[i - 1], TokenLabel::Different});
            --i;
            --j;
        } else if (i > 0 && dp[i][j] == dp[i - 1][j] + 1) {
            reverse.push_back({ref[i - 1], TokenLabel::Missing});
            --i;
        } else {
            extraReverse.push_back(hyp[j - 1]);
            --j;
        }
    }

    result.alignedTokens.assign(reverse.rbegin(), reverse.rend());
    result.extraTokens.assign(extraReverse.rbegin(), extraReverse.rend());

    return result;
}

std::string WERScorer::name() const { return "WER"; }
