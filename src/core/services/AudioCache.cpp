#include "internal/AudioCache.h"

#include <QCryptographicHash>
#include <filesystem>

#include "thoth/ConfigStore.h"

namespace fs = std::filesystem;

AudioCache::AudioCache() : m_cacheDir(ConfigStore::instance().getCacheDir()) {
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

std::string AudioCache::_hash(const std::string& input) {
    QByteArray hash =
        QCryptographicHash::hash(QByteArray::fromStdString(input), QCryptographicHash::Md5);
    return hash.toHex().toStdString();
}

std::string AudioCache::_extensionForEncoding(const std::string& cacheIdentity) {
    auto pos = cacheIdentity.rfind('|');
    if (pos == std::string::npos) {
        return ".mp3";
    }
    std::string encoding = cacheIdentity.substr(pos + 1);
    if (encoding == "MP3" || encoding == "mp3") {
        return ".mp3";
    }
    if (encoding == "OGG_OPUS" || encoding == "ogg_opus") {
        return ".ogg";
    }
    if (encoding == "LINEAR16" || encoding == "WAV" || encoding == "wav") {
        return ".wav";
    }
    if (encoding == "MULAW" || encoding == "mulaw") {
        return ".wav";
    }
    return ".mp3";
}

std::optional<fs::path> AudioCache::get(const std::string& sentence) const {
    fs::path dst = getFileName(sentence);
    if (fs::exists(dst) && fs::file_size(dst) > 0) {
        return dst;
    }
    return std::nullopt;
}

std::optional<fs::path> AudioCache::get(int idx) const {
    if (m_idxToPath.contains(idx)) {
        return m_idxToPath.at(idx);
    }
    return std::nullopt;
}

fs::path AudioCache::getFileName(const std::string& sentence) const {
    return m_cacheDir / (_hash(sentence) + ".mp3");
}

fs::path AudioCache::getFileName(const std::string& sentence,
                                 const std::string& cacheIdentity) const {
    std::string key = sentence + "|" + cacheIdentity;
    return m_cacheDir / (_hash(key) + _extensionForEncoding(cacheIdentity));
}

std::optional<fs::path> AudioCache::get(const std::string& sentence,
                                        const std::string& cacheIdentity) const {
    fs::path dst = getFileName(sentence, cacheIdentity);
    if (fs::exists(dst) && fs::file_size(dst) > 0) {
        return dst;
    }
    return std::nullopt;
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

fs::path AudioCache::saveAudio(int idx, const std::string& sentence, const QByteArray& data,
                               const std::string& cacheIdentity) {
    fs::path dst = getFileName(sentence, cacheIdentity);

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
    throw std::runtime_error("Not implemented");
}
