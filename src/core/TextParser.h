#pragma once
#include "FileExtractor.h"
#include "Segmenter.h"
#include <filesystem>
#include <vector>
#include <string>

class TextParser {
    public:
    TextParser() = default;

    Sentences parseFile(const std::filesystem::path& path) {
        auto extractor = createFileExtractor(path);
        std::string content = extractor->extract(path);
        return _segmenter.segment(content);
    };

    private:
    RegexSegmenter _segmenter;
};