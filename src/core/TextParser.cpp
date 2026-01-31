#include "thoth/TextParser.h"

#include "internal/FileExtractor.h"
#include "internal/Segmenter.h"

TextParser::TextParser() : _segmenter(createRegexSegmenter()) {}

TextParser::~TextParser() = default;

std::vector<std::string> TextParser::parseFile(const std::filesystem::path& path) {
    auto extractor = createFileExtractor(path);
    std::string content = extractor->extract(path);
    return _segmenter->segment(content);
}