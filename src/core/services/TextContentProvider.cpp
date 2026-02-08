#include "internal/AudioCache.h"
#include "internal/Q_GCPTTSDownloader.h"
#include "internal/TextParser.h"
#include "thoth/ConfigStore.h"
#include "thoth/ContentProvider.h"
#include "thoth/Logger.h"

TextContentProvider::TextContentProvider()
    : m_ttsDownloader(std::make_shared<Q_GCPTTSDownloader>()),
      m_textParser(std::make_shared<TextParser>()) {
    // Audio Cache is not Qt-based, using smartpointer
    auto cacheBaseDir = ConfigStore::instance().getCacheDir();
    auto cacheDir = cacheBaseDir / "audio" / ConfigStore::instance().getGoogleTTSConfig().voiceName;
    m_audioCache = std::make_unique<AudioCache>(cacheDir);
}

TextContentProvider::~TextContentProvider() = default;

std::string TextContentProvider::prefix() { return "text://"; }

void TextContentProvider::load(const std::filesystem::path& inputMediaPath,
                               std::function<void(Session)> onReady) {
    auto rawSentences = m_textParser->parseFile(inputMediaPath);

    Session session;
    session.title = inputMediaPath.stem().string();
    session.inputMediaPath = prefix() + inputMediaPath.string();

    for (size_t i = 0; i < rawSentences.size(); ++i) {
        session.sentences.push_back(Sentence{
            .id = std::to_string(i),
            .text = rawSentences[i],
        });
    }

    LOG_INFO("Loaded {} sentences from {}", rawSentences.size(), inputMediaPath.string());

    if (onReady) {
        onReady(session);
    }
}

void TextContentProvider::prepareAudio(Sentence& sentence,
                                       std::function<void(bool success)> callback) {
    // 1. Check if the audio is already cached
    if (auto p = m_audioCache->get(sentence.text)) {
        sentence.localAudioPath = p->string();
        LOG_DEBUG("Audio for sentence [{}] is already cached at {}", sentence.id,
                  sentence.localAudioPath.string());
        callback(true);
        return;
    }

    // 2. Check if the audio is being downloaded
    if (m_downloadingIdx.contains(sentence.id)) {
        LOG_DEBUG("Audio for sentence [{}] is already being downloaded", sentence.id);
        return;
    }

    // 3. Download and save
    m_downloadingIdx.insert(sentence.id);
    try {
        m_ttsDownloader->download(
            sentence.text, [this, &sentence, callback](bool success, QByteArray data) {
                if (success) {
                    auto p = m_audioCache->saveAudio(std::stoi(sentence.id), sentence.text, data);
                    sentence.localAudioPath = p.string();
                    LOG_DEBUG("Downloaded audio for sentence [{}] to {}", sentence.id,
                              sentence.localAudioPath.string());
                    callback(true);
                } else {
                    LOG_ERROR("Failed to download audio for sentence [{}]", sentence.id);
                    callback(false);
                }
                m_downloadingIdx.erase(sentence.id);
            });
    } catch (const std::exception& e) {
        LOG_ERROR(
            "Exception occured in downloading thread due to {}, current Text Content Provider "
            "status is {}",
            e.what(), *this);
        m_downloadingIdx.erase(sentence.id);
        callback(false);
    }
}

std::ostream& operator<<(std::ostream& os, const TextContentProvider& contentProvider) {
    os << "TextContentProvider: {"
       << "Downloading: " << contentProvider.m_downloadingIdx.size() << ", "
       << "CacheDir: " << contentProvider.m_audioCache->getCacheDir().string() << "}" << std::endl;
    return os;
};

void TextContentProvider::dumpState() const { LOG_INFO("Current Status: {}", *this); }