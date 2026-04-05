#include <gtest/gtest.h>

#include "thoth/WERScorer.h"

TEST(WERScorerTest, PerfectMatch) {
    WERScorer scorer;
    EXPECT_DOUBLE_EQ(scorer.score("hello world", "hello world"), 1.0);
}

TEST(WERScorerTest, TotalMismatch) {
    WERScorer scorer;
    // "abc" vs "xyz" — 1 substitution / 1 reference word = WER 1.0 → score 0.0
    EXPECT_DOUBLE_EQ(scorer.score("abc", "xyz"), 0.0);
}

TEST(WERScorerTest, PartialMatch) {
    WERScorer scorer;
    // ref: "the cat sat", hyp: "the cat"
    // edit distance = 1 deletion / 3 ref words = 0.333... WER → score ~0.667
    double s = scorer.score("the cat sat", "the cat");
    EXPECT_NEAR(s, 2.0 / 3.0, 1e-9);
}

TEST(WERScorerTest, CaseInsensitive) {
    WERScorer scorer;
    EXPECT_DOUBLE_EQ(scorer.score("Hello World", "hello world"), 1.0);
}

TEST(WERScorerTest, PunctuationIgnored) {
    WERScorer scorer;
    // punctuation stripped, then compared
    EXPECT_DOUBLE_EQ(scorer.score("Hello, world!", "hello world"), 1.0);
}

TEST(WERScorerTest, EmptyHypothesis) {
    WERScorer scorer;
    // all ref words deleted → WER = 1.0 → score = 0.0
    EXPECT_DOUBLE_EQ(scorer.score("hello world", ""), 0.0);
}

TEST(WERScorerTest, BothEmpty) {
    WERScorer scorer;
    EXPECT_DOUBLE_EQ(scorer.score("", ""), 1.0);
}

TEST(WERScorerTest, ScoreClampedToZero) {
    WERScorer scorer;
    // hypothesis longer than reference — WER can exceed 1.0, clamp to 0
    double s = scorer.score("hi", "one two three four five");
    EXPECT_GE(s, 0.0);
    EXPECT_LE(s, 1.0);
}

TEST(WERScorerTest, NameReturnsWER) {
    WERScorer scorer;
    EXPECT_EQ(scorer.name(), "WER");
}
