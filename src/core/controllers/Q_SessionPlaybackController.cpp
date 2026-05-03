#include <thoth/Logger.h>
#include <thoth/Q_SessionPlaybackController.h>

#include <QDebug>
#include <QUrl>
#include <iostream>

#include "thoth/ContentProvider.h"
#include "thoth/Q_AudioPlayer.h"

Q_SessionPlaybackController::Q_SessionPlaybackController(Q_AudioPlayer* audioPlayer,
                                                         IContentProvider* contentProvider,
                                                         QObject* parent)
    : QObject(parent), m_audioPlayer(audioPlayer), m_contentProvider(contentProvider) {
    m_loopTimer = std::make_unique<QTimer>(this);
    m_loopTimer->setSingleShot(true);
    _connectPlayer();
}

Q_SessionPlaybackController::~Q_SessionPlaybackController() { _disconnectPlayer(); }

void Q_SessionPlaybackController::_connectPlayer() {
    connect(m_audioPlayer, &Q_AudioPlayer::finished, this,
            &Q_SessionPlaybackController::_onPlayerFinished);
    connect(m_audioPlayer, &Q_AudioPlayer::errorOccurred, this,
            &Q_SessionPlaybackController::_onPlayerError);
    connect(m_loopTimer.get(), &QTimer::timeout, this,
            &Q_SessionPlaybackController::_onLoopTimerTimeout);
    m_active = true;
}

void Q_SessionPlaybackController::_disconnectPlayer() {
    disconnect(m_audioPlayer, &Q_AudioPlayer::finished, this,
               &Q_SessionPlaybackController::_onPlayerFinished);
    disconnect(m_audioPlayer, &Q_AudioPlayer::errorOccurred, this,
               &Q_SessionPlaybackController::_onPlayerError);
    disconnect(m_loopTimer.get(), &QTimer::timeout, this,
               &Q_SessionPlaybackController::_onLoopTimerTimeout);
    m_active = false;
}

void Q_SessionPlaybackController::setSession(const Session& session) {
    stop();
    m_session = session;
    m_currentIdx = DEFAULT_START_IDX;
}

void Q_SessionPlaybackController::setContentProvider(IContentProvider* contentProvider) {
    stop();
    m_contentProvider = contentProvider;
}

void Q_SessionPlaybackController::reset() {
    stop();
    m_currentIdx = DEFAULT_START_IDX;
}

void Q_SessionPlaybackController::play() {
    if (!m_autoAdvance) {
        // User audio is active (playing or ended at EndOfMedia).
        // Stop it immediately and let the session take over.
        m_audioPlayer->stop();
    } else if (m_audioPlayer->playbackState() == QMediaPlayer::PlayingState) {
        return;  // Session-driven playback already in progress.
    }
    playSentence(m_currentIdx);
}

void Q_SessionPlaybackController::suspendAutoAdvance() { m_autoAdvance = false; }

void Q_SessionPlaybackController::playSentence(int idx) {
    m_autoAdvance = true;
    if (idx < 0 || idx >= static_cast<int>(m_session.sentences.size())) {
        LOG_WARN("Invalid index [{}], expected range [0, {})", idx, m_session.sentences.size());
        return;
    }

    // if the current sentence is paused, resume it
    if (idx == m_currentIdx && m_audioPlayer->playbackState() == QMediaPlayer::PausedState) {
        resume();
        return;
    }

    // the target sentence is another one, set by idx
    stop();
    m_currentIdx = idx;
    emit statusMessageChanged(QString("Playing sentence [%1]").arg(idx));

    // Try get the audio
    Sentence& sentence = m_session.sentences[idx];
    m_contentProvider->prepareAudio(sentence, [this, idx](bool success, const QString& errorMsg) {
        // if the target sentence is set to others
        if (idx != m_currentIdx || !m_active) {
            LOG_INFO("Ignore the stale playback callback sentence [{}]", idx);
            return;
        }

        if (success) {
            m_audioPlayer->play(m_session.sentences[idx].localAudioPath);
            emit sentencePlayStarted(idx);
            _prefetchNext(idx);
        } else {
            LOG_ERROR("Failed to prepare audio for sentence [{}]", idx);
            QString detail = errorMsg.isEmpty()
                                 ? QString("Failed to prepare audio for sentence [%1]").arg(idx)
                                 : errorMsg;
            emit errorOccurred(detail);
            emit statusMessageChanged(
                QString("Failed to prepare audio for sentence [%1]").arg(idx));
        }
    });
}

void Q_SessionPlaybackController::_prefetchNext(int start_idx) {
    for (size_t i = 1; start_idx + i < m_session.sentences.size() && i <= PRELOAD_WINDOW; ++i) {
        int next_idx = start_idx + i;
        Sentence& sentence = m_session.sentences[next_idx];
        m_contentProvider->prepareAudio(sentence, [next_idx](bool success, const QString&) {
            if (!success) {
                LOG_ERROR("Failed to prefetch audio for sentence [{}]", next_idx);
                return;
            }
        });
    }
}

void Q_SessionPlaybackController::playNext() {
    if (m_currentIdx < static_cast<int>(m_session.sentences.size()) - 1) {
        playSentence(m_currentIdx + 1);
    } else {
        stop();
        emit statusMessageChanged(QString("Reached the end of the session"));
    }
}

void Q_SessionPlaybackController::playPrevious() {
    if (m_currentIdx > 0) {
        playSentence(m_currentIdx - 1);
    } else {
        stop();
        emit statusMessageChanged(QString("Reached the beginning of the session"));
    }
}

void Q_SessionPlaybackController::pause() {
    m_audioPlayer->pause();
    m_loopTimer->stop();
    emit sentencePlayPaused(m_currentIdx);
}

void Q_SessionPlaybackController::resume() { m_audioPlayer->resume(); }

void Q_SessionPlaybackController::stop() {
    m_audioPlayer->stop();
    m_loopTimer->stop();
    emit sentencePlayStopped(m_currentIdx);
    emit statusMessageChanged(QString("Stopped sentence [%1]").arg(m_currentIdx));
}

void Q_SessionPlaybackController::setLoopSingle(bool enable, int delaySeconds) {
    m_singleLoop = enable;
    m_loopDelaySeconds = delaySeconds;
}

void Q_SessionPlaybackController::setMode(const std::string& mode) { m_mode = mode; }

std::string Q_SessionPlaybackController::mode() const { return m_mode; }

int Q_SessionPlaybackController::currentIndex() const { return m_currentIdx; }
bool Q_SessionPlaybackController::isActive() const { return m_active; }

void Q_SessionPlaybackController::_onPlayerFinished() {
    if (!m_autoAdvance) return;
    emit sentencePlayFinished(m_currentIdx);

    if (m_mode == "pause-and-repeat") {
        m_autoAdvance = false;
        emit repeatRequested(m_currentIdx);
        return;
    }

    if (m_loopDelaySeconds > 0) {
        // delay between current and the next play
        m_loopTimer->start(m_loopDelaySeconds * 1000);
    } else {
        // no delay, play the next sentence immediately
        _onLoopTimerTimeout();
    }
}

void Q_SessionPlaybackController::_onPlayerError(const QString& errorMsg) {
    LOG_ERROR("Error occurred in playback: {}", errorMsg.toStdString());
    emit errorOccurred(QString("Error occurred: %1").arg(errorMsg));
}

void Q_SessionPlaybackController::_onLoopTimerTimeout() {
    if (m_singleLoop) {
        playSentence(m_currentIdx);
    } else {
        playNext();
    }
}

std::ostream& operator<<(std::ostream& os, const Q_SessionPlaybackController& controller) {
    os << "PlaybackControllerState: {"
       << "Index: " << controller.m_currentIdx << "/" << controller.m_session.sentences.size()
       << ", "
       << "State: " << controller.m_audioPlayer->playbackState() << ", "
       << "LoopSingle: " << (controller.m_singleLoop ? "Yes" : "No") << ", "
       << "LoopDelay: " << controller.m_loopDelaySeconds << "s" << "}" << std::endl;
    return os;
}

QDebug operator<<(QDebug dbg, const Q_SessionPlaybackController& controller) {
    dbg.nospace() << "SessionPlaybackControllerState: {"
                  << "Index: " << controller.m_currentIdx << "/"
                  << controller.m_session.sentences.size() << ", "
                  << "State: " << controller.m_audioPlayer->playbackState() << ", "
                  << "LoopSingle: " << (controller.m_singleLoop ? "Yes" : "No") << ", "
                  << "LoopDelay: " << controller.m_loopDelaySeconds << "s"
                  << "}";
    return dbg;
}

void Q_SessionPlaybackController::dumpState() const { LOG_INFO("Current Status: {}", *this); }
