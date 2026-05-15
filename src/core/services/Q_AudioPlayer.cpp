#include "thoth/Q_AudioPlayer.h"

#include <algorithm>

#include "thoth/Logger.h"

Q_AudioPlayer::Q_AudioPlayer(QObject* parent)
    : QObject(parent),
      m_player(new QMediaPlayer(this)),
      m_audioOutput(new QAudioOutput(this)),
      m_rangeTimer(new QTimer(this)) {
    m_player->setAudioOutput(m_audioOutput.get());
    m_rangeTimer->setSingleShot(true);
    connect(m_rangeTimer, &QTimer::timeout, this, &Q_AudioPlayer::_onRangeTimerTimeout);
    _setupConnections();
}

Q_AudioPlayer::~Q_AudioPlayer() { stop(); }

void Q_AudioPlayer::_setupConnections() {
    connect(m_player.get(), &QMediaPlayer::mediaStatusChanged, this,
            &Q_AudioPlayer::_onMediaStatusChanged);
    connect(m_player.get(), &QMediaPlayer::errorOccurred, this, &Q_AudioPlayer::_onErrorOccurred);
    connect(m_player.get(), &QMediaPlayer::positionChanged, this,
            &Q_AudioPlayer::_onPositionChanged);
}

QString Q_AudioPlayer::toQString(const std::filesystem::path& path) {
#ifdef _WIN32
    return QString::fromStdWString(path.wstring());
#else
    return QString::fromStdString(path.string());
#endif
}

void Q_AudioPlayer::play(const std::filesystem::path& audioPath) {
    _clearRangeState();
    m_player->stop();
    m_player->setSource(QUrl::fromLocalFile(toQString(audioPath)));
    m_player->play();
    emit started();
}

void Q_AudioPlayer::play(const std::filesystem::path& audioPath, double startMs, double endMs) {
    if (endMs <= startMs) {
        emit errorOccurred(QString("Invalid playback range: %1-%2 ms").arg(startMs).arg(endMs));
        return;
    }

    const QUrl targetUrl = QUrl::fromLocalFile(toQString(audioPath));
    _clearRangeState();
    m_player->stop();

    m_hasPendingRange = true;
    m_pendingRangeUrl = targetUrl;
    m_pendingRangeStartMs = startMs;
    m_pendingRangeEndMs = endMs;

    const auto status = m_player->mediaStatus();
    if (m_hasPendingRange && m_player->source() == targetUrl &&
        (_isRangePlayableStatus(status) || status == QMediaPlayer::EndOfMedia)) {
        LOG_INFO("Range playback: same source already loaded, start immediately: {} [{}-{}]",
                 targetUrl.toString().toStdString(), startMs, endMs);
        _startRangePlayback(startMs, endMs);
    } else if (m_hasPendingRange) {
        LOG_INFO("Range playback: loading source: {} [{}-{}]", targetUrl.toString().toStdString(),
                 startMs, endMs);
        m_player->setSource(targetUrl);
    }

    emit started();
}

void Q_AudioPlayer::pause() {
    if (m_player->playbackState() == QMediaPlayer::PlayingState) {
        m_player->pause();
    }
    emit paused();
}

void Q_AudioPlayer::resume() {
    if (m_player->playbackState() != QMediaPlayer::PlayingState) {
        m_player->play();
    }
    emit resumed();
}

void Q_AudioPlayer::stop() {
    _clearRangeState();
    m_player->stop();
    emit stopped();
}

bool Q_AudioPlayer::isPlaying() const {
    return m_player->playbackState() == QMediaPlayer::PlayingState;
}

bool Q_AudioPlayer::isPaused() const {
    return m_player->playbackState() == QMediaPlayer::PausedState;
}

bool Q_AudioPlayer::isStopped() const {
    return m_player->playbackState() == QMediaPlayer::StoppedState;
}

QMediaPlayer::PlaybackState Q_AudioPlayer::playbackState() const {
    return m_player->playbackState();
}

void Q_AudioPlayer::setRate(qreal rate) { m_player->setPlaybackRate(rate); }

qreal Q_AudioPlayer::rate() const { return m_player->playbackRate(); }

void Q_AudioPlayer::_onMediaStatusChanged(QMediaPlayer::MediaStatus status) {
    LOG_DEBUG("Media status changed: {}", [](QMediaPlayer::MediaStatus status) -> std::string {
        switch (status) {
            case QMediaPlayer::MediaStatus::LoadedMedia:
                return "LoadedMedia";
            case QMediaPlayer::MediaStatus::BufferedMedia:
                return "BufferedMedia";
            case QMediaPlayer::MediaStatus::EndOfMedia:
                return "EndOfMedia";
            case QMediaPlayer::MediaStatus::InvalidMedia:
                return "InvalidMedia";
            case QMediaPlayer::MediaStatus::LoadingMedia:
                return "LoadingMedia";
            case QMediaPlayer::MediaStatus::StalledMedia:
                return "StalledMedia";
            case QMediaPlayer::MediaStatus::BufferingMedia:
                return "BufferingMedia";
            default:
                return "Unknown";
        }
    }(status));

    if (m_hasPendingRange && _isRangePlayableStatus(status) &&
        m_player->source() == m_pendingRangeUrl) {
        _startRangePlayback(m_pendingRangeStartMs, m_pendingRangeEndMs);
    }

    if (status == QMediaPlayer::EndOfMedia) {
        _clearRangeState();
        emit finished();
    }

    if (status == QMediaPlayer::InvalidMedia && m_hasPendingRange) {
        _clearRangeState();
        emit errorOccurred("Invalid media for range playback");
    }
}

void Q_AudioPlayer::_onPositionChanged(qint64 positionMs) {
    if (m_rangeEndMs >= 0.0 && static_cast<double>(positionMs) >= m_rangeEndMs) {
        _finishRangePlayback();
    }
}

void Q_AudioPlayer::_onRangeTimerTimeout() {
    if (m_rangeEndMs >= 0.0) {
        _finishRangePlayback();
    }
}

void Q_AudioPlayer::_onErrorOccurred(QMediaPlayer::Error error, const QString& errorMessage) {
    (void)error;
    LOG_ERROR("Error occurred in audio player: {}", errorMessage.toStdString());
    _clearRangeState();
    emit errorOccurred(errorMessage);
}

void Q_AudioPlayer::_clearRangeState() {
    m_rangeTimer->stop();
    m_hasPendingRange = false;
    m_pendingRangeUrl = QUrl();
    m_pendingRangeStartMs = -1.0;
    m_pendingRangeEndMs = -1.0;
    m_rangeEndMs = -1.0;
}

bool Q_AudioPlayer::_isRangePlayableStatus(QMediaPlayer::MediaStatus status) const {
    return status == QMediaPlayer::LoadedMedia || status == QMediaPlayer::BufferedMedia ||
           status == QMediaPlayer::BufferingMedia;
}

void Q_AudioPlayer::_startRangePlayback(double startMs, double endMs) {
    m_hasPendingRange = false;
    m_pendingRangeUrl = QUrl();
    m_pendingRangeStartMs = -1.0;
    m_pendingRangeEndMs = -1.0;
    m_rangeEndMs = endMs;

    LOG_DEBUG("Range playback: start [{}-{}]", startMs, endMs);
    m_player->setPosition(static_cast<qint64>(startMs));
    m_player->play();

    const qreal playbackRate = std::max<qreal>(0.01, m_player->playbackRate());
    const auto fallbackMs = static_cast<int>((endMs - startMs) / playbackRate) + 1000;
    m_rangeTimer->start(std::max(0, fallbackMs));
}

void Q_AudioPlayer::_finishRangePlayback() {
    _clearRangeState();
    m_player->stop();
    emit finished();
}
