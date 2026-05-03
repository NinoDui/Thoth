#pragma once
#include <QAudioOutput>
#include <QMediaPlayer>
#include <QObject>
#include <QTimer>
#include <filesystem>
#include <memory>

class Q_AudioPlayer : public QObject {
    Q_OBJECT
   public:
    explicit Q_AudioPlayer(QObject* parent = nullptr);
    ~Q_AudioPlayer();

    void play(const std::filesystem::path& audioPath);
    void play(const std::filesystem::path& audioPath, double startMs, double endMs);
    void pause();
    void resume();
    void stop();

    void setRate(qreal rate);
    [[nodiscard]] qreal rate() const;

    [[nodiscard]] bool isPlaying() const;
    [[nodiscard]] bool isPaused() const;
    [[nodiscard]] bool isStopped() const;
    [[nodiscard]] QMediaPlayer::PlaybackState playbackState() const;

   signals:
    void started();
    void paused();
    void resumed();
    void stopped();
    void finished();
    void errorOccurred(const QString& errorMessage);

   private slots:
    void _onMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void _onErrorOccurred(QMediaPlayer::Error error, const QString& errorMessage);
    void _onRangeTimerTimeout();

   private:
    static QString toQString(const std::filesystem::path& path);
    void _setupConnections();

    std::unique_ptr<QMediaPlayer> m_player;
    std::unique_ptr<QAudioOutput> m_audioOutput;
    QTimer* m_rangeTimer;

    // -1.0 when not in range-play mode; >= 0 when a pending seek is queued
    double m_rangeStartMs = -1.0;
    double m_rangeEndMs = -1.0;
};
