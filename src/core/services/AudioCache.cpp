#include "internal/AudioCache.h"

#include <QCryptographicHash>
#include <filesystem>

#include "thoth/ConfigStore.h"

namespace fs = std::filesystem;

AudioCache::AudioCache() : m_cacheDir(ConfigStore::instance().getCacheDir()), m_override(false) {
    if (!fs::exists(m_cacheDir) || (m_override && fs::exists(m_cacheDir))) {
        fs::create_directories(m_cacheDir);
    }
}

AudioCache::AudioCache(const fs::path& cache_dir, bool override)
    : m_cacheDir(cache_dir), m_override(override) {
    if (!fs::exists(m_cacheDir) || (m_override && fs::exists(m_cacheDir))) {
        fs::create_directories(m_cacheDir);
    }
}

AudioCache::~AudioCache() = default;

std::string AudioCache::_hash(const std::string& sentence) const {
    QByteArray hash =
        QCryptographicHash::hash(QByteArray::fromStdString(sentence), QCryptographicHash::Md5);
    return hash.toHex().toStdString();
}

std::optional<fs::path> AudioCache::get(const std::string& sentence) const {
    fs::path dst = getFileName(sentence);
    if (fs::exists(dst) && fs::file_size(dst) > 0) return dst;
    return std::nullopt;
}

std::optional<fs::path> AudioCache::get(int idx) const {
    if (m_idxToPath.find(idx) != m_idxToPath.end()) return m_idxToPath.at(idx);
    return std::nullopt;
}

fs::path AudioCache::getFileName(const std::string& sentence) const {
    return m_cacheDir / (_hash(sentence) + ".mp3");
}

fs::path AudioCache::saveAudio(int idx, const std::string& sentence, const QByteArray& data) {
    fs::path dst = getFileName(sentence);

    QFile dstFile(QString::fromStdString(dst.string()));
    if (dstFile.open(QIODevice::WriteOnly)) {
        dstFile.write(data);
        dstFile.close();
        m_idxToPath[idx] = dst;
    } else {
        throw std::runtime_error("Failed to save audio to " + dst.string());
    }
    return dst;
}

fs::path AudioCache::getCacheDir() const { return m_cacheDir; }

void AudioCache::exportAudioToOne(const fs::path& dstPath) const {
    // TODO: implement the save to all method
    throw std::runtime_error("Not implemented");
}