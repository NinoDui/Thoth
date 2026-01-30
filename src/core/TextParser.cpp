#include <TextParser.h>

Sentences TextParser::parseFile(const std::filesystem::path& path) {
    auto extractor = createFileExtractor(path);
    std::string content = extractor->extract(path);
    return _segmenter.segment(content);
}