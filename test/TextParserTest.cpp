#include <gtest/gtest.h>
#include <core/TextParser.h>


TEST(TextParserTest, HandleEmptyFile) {
    TextParser parser;
    EXPECT_THROW(parser.parseFile(""), std::invalid_argument);
}

TEST(TextParserTest, HandleValidContent) {
    TextParser parser;

    std::string content = "Hello, Mayday! Ashin. Ming. Monster. Masa. Stone.";
    auto sentences = parser.parseFile(content);

    EXPECT_EQ(sentences.size(), 6);
}