#pragma once
#include <QByteArray>
#include <QFile>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>

class ConfigStore;

class AudioCache {
   public:
    AudioCache();
    explicit AudioCache(const std::filesystem::path& cache_dir, bool override = false);
    ~AudioCache();

    std::optional<std::filesystem::path> get(const std::string& sentence) const;
    std::optional<std::filesystem::path> get(int idx) const;

    std::filesystem::path getFileName(const std::string& sentence) const;
    std::filesystem::path getFileName(const std::string& sentence,
                                      const std::string& cacheIdentity) const;

    std::optional<std::filesystem::path> get(const std::string& sentence,
                                             const std::string& cacheIdentity) const;

    std::filesystem::path getCacheDir() const;
    std::filesystem::path saveAudio(int idx, const std::string& sentence, const QByteArray& data);
    std::filesystem::path saveAudio(int idx, const std::string& sentence, const QByteArray& data,
                                    const std::string& cacheIdentity);
    void exportAudioToOne(const std::filesystem::path& dstPath) const;

   private:
    std::filesystem::path m_cacheDir;
    bool m_override = false;
    std::unordered_map<int, std::filesystem::path> m_idxToPath;

    static std::string _hash(const std::string& input);
    static std::string _extensionForEncoding(const std::string& cacheIdentity);
};
