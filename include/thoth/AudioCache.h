#pragma once
#include <filesystem>
#include <optional>

class AudioCache {
public:
    // "explicit" to avoid shadowing convert
    explicit AudioCache(const std::filesystem::path& cache_dir = "cache", bool override = false);

    // foo () const; -> READ ONLY
    std::optional<std::filesystem::path> get(const std::string& sentence) const;

    std::filesystem::path getDst(const std::string& sentence) const;

private:
    std::filesystem::path m_cacheDir;
    bool m_override = false;

    std::string _hash(const std::string& sentence) const;
};