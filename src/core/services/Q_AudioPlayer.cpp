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
}

QString Q_AudioPlayer::toQString(const std::filesystem::path& path) {
#ifdef _WIN32
    return QString::fromStdWString(path.wstring());
#else
    return QString::fromStdString(path.string());
#endif
}

void Q_AudioPlayer::play(const std::filesystem::path& audioPath) {
    m_rangeTimer->stop();
    m_rangeStartMs = -1.0;
    m_rangeEndMs = -1.0;
    m_player->stop();
    m_player->setSource(QUrl::fromLocalFile(toQString(audioPath)));
    m_player->play();
    emit started();
}

void Q_AudioPlayer::play(const std::filesystem::path& audioPath, double startMs, double endMs) {
    m_rangeTimer->stop();
    m_rangeStartMs = startMs;
    m_rangeEndMs = endMs;
    m_player->stop();
    m_player->setSource(QUrl::fromLocalFile(toQString(audioPath)));
    // Actual play() + seek deferred to _onMediaStatusChanged(LoadedMedia)
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
    m_rangeTimer->stop();
    m_rangeStartMs = -1.0;
    m_rangeEndMs = -1.0;
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

    if (status == QMediaPlayer::LoadedMedia && m_rangeStartMs >= 0.0) {
        m_player->setPosition(static_cast<qint64>(m_rangeStartMs));
        m_player->play();
        auto durationMs = static_cast<int>(m_rangeEndMs - m_rangeStartMs);
        m_rangeTimer->start(std::max(0, durationMs));
        m_rangeStartMs = -1.0;  // consumed
    }

    if (status == QMediaPlayer::EndOfMedia) {
        m_rangeTimer->stop();
        m_rangeEndMs = -1.0;
        emit finished();
    }
}

void Q_AudioPlayer::_onRangeTimerTimeout() {
    m_rangeEndMs = -1.0;
    m_player->stop();
    emit finished();
}

void Q_AudioPlayer::_onErrorOccurred(QMediaPlayer::Error error, const QString& errorMessage) {
    LOG_ERROR("Error occurred in audio player: {}", errorMessage.toStdString());
    emit errorOccurred(errorMessage);
}
