#include "FileExtractor.h"
#include <fstream>
#include <stdexcept>
#include <iostream>

class TxtExtractor : public FileExtractor {
    public:
    std::string extract(const std::filesystem::path& path) const override {
        std::ifstream file(path);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file: " + path.string());
        }

        return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    }
};


class PdfExtractor : public FileExtractor {
    public:
    std::string extract(const std::filesystem::path& path) const override {
        throw std::runtime_error("PDF extraction not implemented");
    }
};

std::unique_ptr<FileExtractor> createFileExtractor(const std::string& format) {
    // std::transform(format.begin(), format.end(), format.begin(), [](unsigned char c){
    //     return std::tolower(c);
    // });
    
    if (format == "txt")    return std::make_unique<TxtExtractor>();
    if (format == "pdf")    return std::make_unique<PdfExtractor>();
    throw std::invalid_argument("Unsupported file format: " + format);
}

std::unique_ptr<FileExtractor> createFileExtractor(const std::filesystem::path& path) {
    std::string format = path.extension().string();
    if (format.starts_with(".")) format = format.substr(1);
    
    return createFileExtractor(format);
}