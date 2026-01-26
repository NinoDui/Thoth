#pragma once
#include <QByteArray>
#include <QFile>
#include <filesystem>
#include <optional>
#include <unordered_map>

class AudioCache {
   public:
    // "explicit" to avoid shadowing convert
    explicit AudioCache(const std::filesystem::path& cache_dir = "cache", bool override = false);

    // foo () const; -> READ ONLY
    std::optional<std::filesystem::path> get(const std::string& sentence) const;
    std::optional<std::filesystem::path> get(int idx) const;

    std::filesystem::path getFileName(const std::string& sentence) const;
    std::filesystem::path getCacheDir() const;
    std::filesystem::path saveAudio(int idx, const std::string& sentence, const QByteArray& data);
    void exportAudioToOne(const std::filesystem::path& dstPath) const;

   private:
    std::filesystem::path m_cacheDir;
    bool m_override = false;
    std::unordered_map<int, std::filesystem::path> m_idxToPath;

    std::string _hash(const std::string& sentence) const;
};