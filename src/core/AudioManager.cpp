#include <thoth/AudioManager.h>
#include <thoth/AudioCache.h>
#include <thoth/TTSDownloader.h>
#include <iostream>

AudioManager::AudioManager(QObject* parent)
    : QObject(parent), m_audioCache(std::make_unique<AudioCache>())
    , m_ttsDownloader(new TTSDownloader(this))
    , m_player(new QMediaPlayer(this))
    , m_audioOutput(new QAudioOutput(this)) {
        m_player->setAudioOutput(m_audioOutput);

        connect(m_player, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status) {
            if (status == QMediaPlayer::EndOfMedia) {
                if (m_currentIdx < m_playlist.size() - 1) {
                    play(m_currentIdx + 1);
                }
            }
        });
}

AudioManager::~AudioManager() = default;

void AudioManager::setPlaylist(const std::vector<std::string>& sentences) {
    m_playlist = sentences;
    m_currentIdx = -1;
    m_downloadingIdx.clear();
}

void AudioManager::play(int idx) {
    if (idx < 0 || idx >= m_playlist.size()) return;

    m_currentIdx = idx;
    emit currentSentenceChange(idx);

    prepareAudio(idx, idx == m_currentIdx);
    checkPreload();
}

void AudioManager::prepareAudio(int idx, bool isHighPriority) {
    std::string sentence = m_playlist[idx];

    auto localPath = m_audioCache->get(sentence);
    // hit the cache, ready for play
    if (localPath) {
        if (isHighPriority) {
            playLocalFile(*localPath);
        }
        return;
    }

    // cache missed, but is downloading
    if (m_downloadingIdx.contains(idx)) {
        return;
    }

    m_downloadingIdx.insert(idx);
    auto dstPath = m_audioCache->getDst(sentence);
    m_ttsDownloader->download(sentence, dstPath, [this, idx, isHighPriority](bool success, std::filesystem::path path){
        m_downloadingIdx.erase(idx);
        
        if (success) {
            if (isHighPriority && idx == m_currentIdx) {
                playLocalFile(path);
            }
        } else {
            std::cerr << "Failed to download sentence " << idx << std::endl;
        }
    });
}

void AudioManager::checkPreload() {
    for (int i = 1; i <= PRELOAD_WINDOW; ++i) {
        int nextIdx = m_currentIdx + i;
        if (nextIdx < m_playlist.size()) {
            prepareAudio(nextIdx, false);
        }
    }
}

void AudioManager::playLocalFile(const std::filesystem::path& path) {
    m_player->setSource(QUrl::fromLocalFile(QString::fromStdString(path.string())));
    m_player->play();
}