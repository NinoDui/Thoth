#pragma once
#include <filesystem>
#include <string>
#include <vector>

#include "FileExtractor.h"
#include "Segmenter.h"

class TextParser {
   public:
    TextParser() = default;

    Sentences parseFile(const std::filesystem::path& path);

   private:
    RegexSegmenter _segmenter;
};