#include <thoth/Logger.h>
#include <thoth/Q_AudioManager.h>

#include <iostream>

#include "internal/AudioCache.h"
#include "internal/Q_GCPTTSDownloader.h"

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
                LOG_ERROR("MediaPlayer error (Code {}): {}", (int)error, errorMsg.toStdString());
                LOG_WARNING(
                    "Due to an unexpected error occured, skipping to the next sencence [{}]",
                    m_singleLoop ? m_curIdx : m_curIdx + 1);

                if (m_curIdx < m_playlist.size() - 1) {
                    QTimer::singleShot(0, this, [this]() { playNext(); });
                } else {
                    stop();
                    emit playbackFinished(m_curIdx);
                }
            });
}

Q_AudioManager::~Q_AudioManager() = default;

std::filesystem::path Q_AudioManager::getCacheDir() const { return m_audioCache->getCacheDir(); }

void Q_AudioManager::play(int idx) {
    if (idx < 0 || idx >= m_playlist.size()) {
        LOG_WARNING("Invalid index [{}], expected range [0, {})", idx, m_playlist.size());
        return;
    }

    m_curIdx = idx;
    prepareAndPlay(idx);
    fetchNext(idx);
}

void Q_AudioManager::reset() {
    stop();
    m_curIdx = DEFAULT_START_IDX;
    m_downloadingIdx.clear();
}

void Q_AudioManager::setPlaylist(const std::vector<std::string>& playlist) {
    reset();
    m_playlist = playlist;
}

void Q_AudioManager::pause() { m_player->pause(); }

void Q_AudioManager::resume() {
    LOG_DEBUG("Resume is called, current index: {}", m_curIdx);
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

    if (localPath) {
        LOG_DEBUG("Playing sentence [{}] from cache", idx);
        _Q_play(idx, localPath.value());
        return;
    }

    if (m_downloadingIdx.contains(idx)) {
        LOG_INFO("Sentence [{}] is already being downloaded, waiting for completion", idx);
        return;
    }

    m_downloadingIdx.insert(idx);
    std::string sentence = m_playlist[idx];
    try {
        m_ttsDownloader->download(sentence, [this, idx, sentence](bool success, QByteArray data) {
            if (success) {
                auto p = m_audioCache->saveAudio(idx, sentence, data);
                LOG_DEBUG("Downloaded sentence [{}] to {}", idx, p.string());
                if (idx == m_curIdx) _Q_play(idx, p);
            } else {
                LOG_ERROR("TTS Client failed to download sentence [{}]", idx);
            }
            m_downloadingIdx.erase(idx);
        });
    } catch (const std::exception& e) {
        LOG_ERROR(
            "Exception occured in downloading thread due to {}, current Audio Manager status is {}",
            e.what(), *this);
        m_downloadingIdx.erase(idx);
    }
}

void Q_AudioManager::fetchNext(int start_idx, int window_size) {
    for (int i = 1; start_idx + i < m_playlist.size() && i <= window_size; ++i) {
        int next_idx = start_idx + i;

        if (m_audioCache->get(next_idx)) continue;
        if (m_downloadingIdx.contains(next_idx)) continue;

        m_downloadingIdx.insert(next_idx);
        try {
            m_ttsDownloader->download(
                m_playlist[next_idx], [this, next_idx](bool success, QByteArray data) {
                    if (success) {
                        m_audioCache->saveAudio(next_idx, m_playlist[next_idx], data);
                    } else {
                        LOG_ERROR("TTS Client failed to download sentence [{}]", next_idx);
                    }
                    m_downloadingIdx.erase(next_idx);
                });
        } catch (const std::exception& e) {
            LOG_ERROR(
                "Exception occured in downloading thread due to {}, current Audio Manager status "
                "is {}",
                e.what(), *this);
            m_downloadingIdx.erase(next_idx);
        }
    }
}

void Q_AudioManager::_Q_play(int idx, const std::filesystem::path& localPath) {
    QString qPath;
#ifdef _WIN32
    qPath = QString::fromStdWString(localPath.wstring());
#else
    qPath = QString::fromStdString(localPath.string());
#endif
    LOG_DEBUG("Sending file {} to player", qPath.toStdString());
    m_player->setSource(QUrl::fromLocalFile(qPath));
    m_player->play();
    emit playbackStarted(idx);
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