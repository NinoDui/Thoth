#pragma once
#include <filesystem>
#include <memory>
#include <string>

struct FileExtractor {
    virtual ~FileExtractor() = default;

    [[nodiscard]] virtual std::string extract(const std::filesystem::path& path) const = 0;
};

std::unique_ptr<FileExtractor> createFileExtractor(const std::string& input_format);

std::unique_ptr<FileExtractor> createFileExtractor(const std::filesystem::path& path);
