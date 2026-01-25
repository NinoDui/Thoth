#pragma once
#include "FileExtractor.h"
#include "Segmenter.h"
#include <filesystem>
#include <vector>
#include <string>

class TextParser {
    public:
    TextParser() = default;

    Sentences parseFile(const std::filesystem::path& path);

    private:
    RegexSegmenter _segmenter;
};