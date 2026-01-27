#include <thoth/AudioCache.h>
#include <thoth/Q_AudioManager.h>
#include <thoth/Q_TTSDownloader.h>

#include <iostream>

AudioManager::AudioManager(QObject* parent)
    : QObject(parent),
      m_audioCache(std::make_unique<AudioCache>()),
      // objects below are managed by Qt, safe op using 'new'
      m_player(new QMediaPlayer(this)),
      m_audioOutput(new QAudioOutput(this)),
      m_delayTimer(new QTimer(this)),
      m_ttsDownloader(new TTSDownloader(this)) {
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
}

AudioManager::~AudioManager() = default;

void AudioManager::play(int idx) {
    if (idx < 0 || idx >= m_playlist.size()) return;

    m_curIdx = idx;
    prepareAndPlay(idx);

    fetchNext(idx);
}

void AudioManager::setPlaylist(const std::vector<std::string>& playlist) {
    stop();
    m_playlist = playlist;
    m_curIdx = -1;
    m_downloadingIdx.clear();
}

void AudioManager::pause() { m_player->pause(); }

void AudioManager::resume() {
    if (m_player->playbackState() == QMediaPlayer::PausedState) {
        m_player->play();
    } else if (m_curIdx >= 0 && m_curIdx < m_playlist.size()) {
        play(m_curIdx);
    }
}

void AudioManager::stop() {
    m_player->stop();
    m_delayTimer->stop();
}

bool AudioManager::isPlaying() const {
    return m_player->playbackState() == QMediaPlayer::PlayingState;
}

void AudioManager::playNext() {
    if (m_curIdx >= 0 && m_curIdx < m_playlist.size() - 1) {
        play(m_curIdx + 1);
        emit currentSentenceChange(m_curIdx + 1);
    }
}

void AudioManager::playPrevious() {
    if (m_curIdx > 0) {
        play(m_curIdx - 1);
        emit currentSentenceChange(m_curIdx - 1);
    }
}

void AudioManager::setLoopSingle(bool enable, int delaySeconds) {
    m_singleLoop = enable;
    m_loopDelaySeconds = delaySeconds;
}

void AudioManager::exportAudioToOne(const std::filesystem::path& dstPath) const {
    m_audioCache->exportAudioToOne(dstPath);
}

void AudioManager::prepareAndPlay(int idx) {
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

void AudioManager::fetchNext(int start_idx, int window_size) {
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