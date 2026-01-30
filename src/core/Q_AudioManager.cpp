#include <thoth/AudioCache.h>
#include <thoth/Logger.h>
#include <thoth/Q_AudioManager.h>

#include <iostream>

#include "Q_GCPTTSDownloader.h"

Q_AudioManager::Q_AudioManager(QObject* parent)
    : QObject(parent),
      m_audioCache(std::make_unique<AudioCache>()),
      // objects below are managed by Qt, safe op using 'new'
      m_player(new QMediaPlayer(this)),
      m_audioOutput(new QAudioOutput(this)),
      m_delayTimer(new QTimer(this)),
      m_ttsDownloader(new Q_GCPTTSDownloader(this)) {
    m_player->setAudioOutput(m_audioOutput);
    m_delayTimer->setSingleShot(true);

    connect(m_player, &QMediaPlayer::mediaStatusChanged, this,
            [this](QMediaPlayer::MediaStatus status) {
                if (status == QMediaPlayer::EndOfMedia) {
                    emit playbackFinished(m_curIdx);

                    if (m_singleLoop) {
                        m_delayTimer->start(m_loopDelaySeconds * 1000);
                    } else {
                        playNext();
                    }
                }
            });

    connect(m_delayTimer, &QTimer::timeout, this, [this]() { play(m_curIdx); });

    connect(m_player, &QMediaPlayer::errorOccurred, this,
            [this](QMediaPlayer::Error error, const QString& errorMsg) {
                LOG_ERROR("MediaPlayer error {}", errorMsg.toStdString());
                LOG_INFO("Skipping to the next sencence, due to error occurred.");
                playNext();
            });
}

Q_AudioManager::~Q_AudioManager() = default;

std::filesystem::path Q_AudioManager::getCacheDir() const { return m_audioCache->getCacheDir(); }

void Q_AudioManager::play(int idx) {
    if (idx < 0 || idx >= m_playlist.size()) return;

    m_curIdx = idx;
    prepareAndPlay(idx);

    fetchNext(idx);
}

void Q_AudioManager::setPlaylist(const std::vector<std::string>& playlist) {
    stop();
    m_playlist = playlist;
    m_curIdx = -1;
    m_downloadingIdx.clear();
}

void Q_AudioManager::pause() { m_player->pause(); }

void Q_AudioManager::resume() {
    if (m_player->playbackState() == QMediaPlayer::PausedState) {
        m_player->play();
    } else if (m_curIdx >= 0 && m_curIdx < m_playlist.size()) {
        play(m_curIdx);
    }
}

void Q_AudioManager::stop() {
    m_player->stop();
    m_delayTimer->stop();
}

bool Q_AudioManager::isPlaying() const {
    return m_player->playbackState() == QMediaPlayer::PlayingState;
}

void Q_AudioManager::playNext() {
    m_player->stop();
    if (m_curIdx >= 0 && m_curIdx < m_playlist.size() - 1) {
        play(m_curIdx + 1);
        emit currentSentenceChange(m_curIdx + 1);
    }
}

void Q_AudioManager::playPrevious() {
    m_player->stop();
    if (m_curIdx > 0) {
        play(m_curIdx - 1);
        emit currentSentenceChange(m_curIdx - 1);
    }
}

void Q_AudioManager::setLoopSingle(bool enable, int delaySeconds) {
    m_singleLoop = enable;
    m_loopDelaySeconds = delaySeconds;
}

void Q_AudioManager::exportAudioToOne(const std::filesystem::path& dstPath) const {
    m_audioCache->exportAudioToOne(dstPath);
}

void Q_AudioManager::prepareAndPlay(int idx) {
    auto localPath = m_audioCache->get(idx);
    // hit the cache, ready for play
    if (localPath) {
        m_player->setSource(QUrl::fromLocalFile(QString::fromStdString(localPath->string())));
        m_player->play();
        emit playbackStarted(idx);
        return;
    }

    // cache missed, but is downloading
    if (m_downloadingIdx.contains(idx)) {
        return;
    }

    m_downloadingIdx.insert(idx);
    std::string sentence = m_playlist[idx];
    m_ttsDownloader->download(sentence, [this, idx, sentence](bool success, QByteArray data) {
        m_downloadingIdx.erase(idx);
        if (success) {
            auto dstPath = m_audioCache->saveAudio(idx, sentence, data);

            if (idx == m_curIdx) {
                m_player->setSource(QUrl::fromLocalFile(QString::fromStdString(dstPath.string())));
                m_player->play();
                emit playbackStarted(idx);
            }
        } else {
            std::cerr << "Failed to download sentence " << idx << std::endl;
        }
    });
}

void Q_AudioManager::fetchNext(int start_idx, int window_size) {
    for (int i = 1; start_idx + i < m_playlist.size() && i <= window_size; ++i) {
        int next_idx = start_idx + i;

        if (!m_audioCache->get(next_idx)) {
            m_ttsDownloader->download(
                m_playlist[next_idx], [this, next_idx](bool success, QByteArray data) {
                    if (success) {
                        m_audioCache->saveAudio(next_idx, m_playlist[next_idx], data);
                    } else {
                        std::cerr << "Failed to download sentence " << next_idx << std::endl;
                    }
                });
        }
    }
}

std::string playbackStateToString(QMediaPlayer::PlaybackState state) {
    switch (state) {
        case QMediaPlayer::PlayingState:
            return "Playing";
        case QMediaPlayer::PausedState:
            return "Paused";
        case QMediaPlayer::StoppedState:
            return "Stopped";
        default:
            return "Unknown";
    }
};

std::ostream& operator<<(std::ostream& os, const Q_AudioManager& audioManager) {
    os << "AudioManagerState: {"
       << "Index: " << audioManager.m_curIdx << "/" << audioManager.m_playlist.size() << ", "
       << "State: " << playbackStateToString(audioManager.m_player->playbackState()) << ", "
       << "LoopSingle: " << (audioManager.m_singleLoop ? "Yes" : "No") << ", "
       << "LoopDelay: " << audioManager.m_loopDelaySeconds << "s, "
       << "Downloading: " << audioManager.m_downloadingIdx.size() << ", "
       << "CacheDir: " << audioManager.m_audioCache->getCacheDir().string() << "}" << std::endl;
    return os;
}

QDebug operator<<(QDebug dbg, const Q_AudioManager& audioManager) {
    dbg.nospace() << "AudioManagerState: {"
                  << "Index: " << audioManager.m_curIdx << "/" << audioManager.m_playlist.size()
                  << ", "
                  << "State: " << audioManager.m_player->playbackState() << ", "
                  << "LoopSingle: " << (audioManager.m_singleLoop ? "Yes" : "No") << ", "
                  << "LoopDelay: " << audioManager.m_loopDelaySeconds << "s"
                  << "Downloading: " << audioManager.m_downloadingIdx.size() << ", ";
    return dbg;
}

void Q_AudioManager::dumpState() const { LOG_INFO("Current Status: {}", *this); }