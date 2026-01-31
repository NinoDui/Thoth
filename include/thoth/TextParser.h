#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

class Segmenter;

class TextParser {
   public:
    TextParser();

    // Segmenter is predeclared in the header file
    // so here we need to AVOID impl but only declare the destructor
    ~TextParser();

    std::vector<std::string> parseFile(const std::filesystem::path& path);

   private:
    std::unique_ptr<Segmenter> _segmenter;
};