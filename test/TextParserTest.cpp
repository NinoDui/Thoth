#include <gtest/gtest.h>

#include <filesystem>

#include "internal/TextParser.h"

namespace fs = std::filesystem;

TEST(TextParserTest, HandleEmptyFile) {
    TextParser parser;
    EXPECT_THROW(parser.parseFile(""), std::invalid_argument);
}

TEST(TextParserTest, HandleShortTxtArticle) {
    fs::path resource_dir = TEST_RESOURCE_DIR;
    fs::path article_path = resource_dir / "short_article.txt";

    TextParser parser;
    auto sentences = parser.parseFile(article_path);

    EXPECT_FALSE(sentences.empty());
    EXPECT_EQ(sentences.size(), 9);
}
