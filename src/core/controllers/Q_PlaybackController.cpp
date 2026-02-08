#include <thoth/Logger.h>
#include <thoth/Q_PlaybackController.h>

#include <QDebug>
#include <QUrl>
#include <iostream>

#include "thoth/ContentProvider.h"

Q_PlaybackController::Q_PlaybackController(QObject* parent)
    : QObject(parent),
      // objects below are managed by Qt, safe op using 'new'
      m_player(new QMediaPlayer(this)),
      m_audioOutput(new QAudioOutput(this)),
      m_delayTimer(new QTimer(this)) {
    m_player->setAudioOutput(m_audioOutput.get());
    m_delayTimer->setSingleShot(true);

    m_contentProvider = std::make_unique<TextContentProvider>();
    m_currentIdx = DEFAULT_START_IDX;

    setupConnections();
}

Q_PlaybackController::Q_PlaybackController(IContentProvider* contentProvider, QObject* parent)
    : QObject(parent),
      m_contentProvider(contentProvider),
      m_player(new QMediaPlayer(this)),
      m_audioOutput(new QAudioOutput(this)),
      m_delayTimer(new QTimer(this)) {
    m_player->setAudioOutput(m_audioOutput.get());
    m_delayTimer->setSingleShot(true);
    setupConnections();
}

Q_PlaybackController::~Q_PlaybackController() { stop(); }

void Q_PlaybackController::setupConnections() {
    // when delay timer timeout, let _onLoopTimerTimeout be called
    connect(m_delayTimer.get(), &QTimer::timeout, this, &Q_PlaybackController::_onLoopTimerTimeout);
    // when media status changed, let _onMediaStatusChanged be called
    connect(m_player.get(), &QMediaPlayer::mediaStatusChanged, this,
            &Q_PlaybackController::_onMediaStatusChanged);
    // when error occurred, let _onErrorOccurred be called
    connect(m_player.get(), &QMediaPlayer::errorOccurred, this,
            &Q_PlaybackController::_onErrorOccurred);
}

void Q_PlaybackController::setSession(const Session& session) {
    stop();
    m_currentSession = session;
    m_currentIdx = DEFAULT_START_IDX;
}

void Q_PlaybackController::playSentence(int idx) {
    if (idx < 0 || idx >= static_cast<int>(m_currentSession.sentences.size())) {
        LOG_WARN("Invalid index [{}], expected range [0, {})", idx,
                 m_currentSession.sentences.size());
        return;
    }

    // if the current sentence is paused, resume it
    if (idx == m_currentIdx && m_player->playbackState() == QMediaPlayer::PausedState) {
        resume();
        return;
    }

    // the target sentence is another one, set by idx
    stop();
    m_currentIdx = idx;
    emit statusMessageChanged(QString("Playing sentence [%1]").arg(idx));

    // Try get the audio
    Sentence& sentence = m_currentSession.sentences[idx];
    m_contentProvider->prepareAudio(sentence, [this, idx](bool success) {
        // if the target sentence is set to others
        if (idx != m_currentIdx) {
            LOG_INFO("Ignore the stale playback callback sentence [{}]", idx);
            return;
        }

        if (success) {
            _Q_play(m_currentSession.sentences[idx].localAudioPath);
            _fetchNext(idx);
        } else {
            LOG_ERROR("Failed to prepare audio for sentence [{}]", idx);
            emit errorOccurred(QString("Failed to prepare audio for sentence [%1]").arg(idx));
            emit statusMessageChanged(
                QString("Failed to prepare audio for sentence [%1]").arg(idx));
        }
    });
}

void Q_PlaybackController::_Q_play(const std::filesystem::path& localPath) {
    if (localPath.empty()) {
        LOG_ERROR("Local path is empty, skipping ...");
        emit errorOccurred(QString("Local path is empty, skipping ..."));
        return;
    }

    QString qPath;
#ifdef _WIN32
    qPath = QString::fromStdWString(localPath.wstring());
#else
    qPath = QString::fromStdString(localPath.string());
#endif
    LOG_DEBUG("Sending file {} to player", qPath.toStdString());

    emit playbackStarted(m_currentIdx);
    emit statusMessageChanged(QString("Playing sentence [%1]").arg(m_currentIdx));
    m_player->setSource(QUrl::fromLocalFile(qPath));
    m_player->play();
}

void Q_PlaybackController::_fetchNext(int start_idx) {
    for (size_t i = 1; start_idx + i < m_currentSession.sentences.size() && i <= windows_size;
         ++i) {
        int next_idx = start_idx + i;
        Sentence& sentence = m_currentSession.sentences[next_idx];
        m_contentProvider->prepareAudio(sentence, [next_idx](bool success) {
            if (!success) {
                LOG_ERROR("Failed to prefetch audio for sentence [{}]", next_idx);
                return;
            }
        });
    }
}

void Q_PlaybackController::reset() {
    stop();
    m_currentIdx = DEFAULT_START_IDX;
}

void Q_PlaybackController::play() {
    if (m_player->playbackState() != QMediaPlayer::PlayingState) {
        playSentence(m_currentIdx);
    }
}

void Q_PlaybackController::pause() {
    if (m_player->playbackState() == QMediaPlayer::PlayingState) {
        m_player->pause();
        emit playbackPaused(m_currentIdx);
        emit statusMessageChanged(QString("Paused sentence [%1]").arg(m_currentIdx));
    }
}

void Q_PlaybackController::resume() {
    LOG_DEBUG("Resume is called, current index: {}", m_currentIdx);
    if (m_player->playbackState() != QMediaPlayer::PlayingState) {
        emit playbackStarted(m_currentIdx);
        emit statusMessageChanged(QString("Playing sentence [%1]").arg(m_currentIdx));
        m_player->play();
    }
}

void Q_PlaybackController::stop() {
    m_player->stop();
    m_delayTimer->stop();
    emit playbackStopped(m_currentIdx);
    emit statusMessageChanged(QString("Stopped sentence [%1]").arg(m_currentIdx));
}

void Q_PlaybackController::playNext() {
    if (m_currentIdx < static_cast<int>(m_currentSession.sentences.size()) - 1) {
        playSentence(m_currentIdx + 1);
    } else {
        stop();
        emit statusMessageChanged(QString("Reached the end of the session"));
    }
}

void Q_PlaybackController::playPrevious() {
    if (m_currentIdx > 0) {
        playSentence(m_currentIdx - 1);
    } else {
        stop();
        emit statusMessageChanged(QString("Reached the beginning of the session"));
    }
}

void Q_PlaybackController::setLoopSingle(bool enable, int delaySeconds) {
    m_singleLoop = enable;
    m_loopDelaySeconds = delaySeconds;
}

bool Q_PlaybackController::isPlaying() const {
    return m_player->playbackState() == QMediaPlayer::PlayingState;
}

int Q_PlaybackController::getCurrentSentenceIndex() const { return m_currentIdx; }

void Q_PlaybackController::_onMediaStatusChanged(QMediaPlayer::MediaStatus status) {
    if (status == QMediaPlayer::EndOfMedia) {
        emit playbackFinished(m_currentIdx);

        if (!m_singleLoop) {
            m_currentIdx += 1;
        }

        if (m_loopDelaySeconds > 0) {
            // delay between current and the next play
            m_delayTimer->start(m_loopDelaySeconds * 1000);
        } else {
            // no delay, play the next sentence immediately
            _onLoopTimerTimeout();
        }
    }
}

void Q_PlaybackController::_onLoopTimerTimeout() {
    if (m_currentIdx >= 0) {
        playSentence(m_currentIdx);
    }
}

void Q_PlaybackController::_onErrorOccurred(QMediaPlayer::Error error, const QString& errorMsg) {
    LOG_ERROR("Error occurred in playback: {} [Code: {}]", errorMsg.toStdString(),
              static_cast<int>(error));
    emit errorOccurred(QString("Error occurred: %1").arg(errorMsg));

    playNext();
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

std::ostream& operator<<(std::ostream& os, const Q_PlaybackController& playbackController) {
    os << "PlaybackControllerState: {"
       << "Index: " << playbackController.m_currentIdx << "/"
       << playbackController.m_currentSession.sentences.size() << ", "
       << "State: " << playbackStateToString(playbackController.m_player->playbackState()) << ", "
       << "LoopSingle: " << (playbackController.m_singleLoop ? "Yes" : "No") << ", "
       << "LoopDelay: " << playbackController.m_loopDelaySeconds << "s" << "}" << std::endl;
    return os;
}

QDebug operator<<(QDebug dbg, const Q_PlaybackController& playbackController) {
    dbg.nospace() << "PlaybackControllerState: {"
                  << "Index: " << playbackController.m_currentIdx << "/"
                  << playbackController.m_currentSession.sentences.size() << ", "
                  << "State: "
                  << playbackStateToString(playbackController.m_player->playbackState()) << ", "
                  << "LoopSingle: " << (playbackController.m_singleLoop ? "Yes" : "No") << ", "
                  << "LoopDelay: " << playbackController.m_loopDelaySeconds << "s"
                  << "}";
    return dbg;
}

void Q_PlaybackController::dumpState() const { LOG_INFO("Current Status: {}", *this); }