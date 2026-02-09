#pragma once
#include <QAudioOutput>
#include <QMediaPlayer>
#include <QObject>
#include <filesystem>
#include <memory>

class Q_AudioPlayer : public QObject {
    /**
     * @brief The Q_AudioPlayer class is pure functional wrapper for QMediaPlayer.
     * It takes in a file path and plays the audio file.
     * only holds path -> QMediaPlayer
     */
    Q_OBJECT
   public:
    explicit Q_AudioPlayer(QObject* parent = nullptr);
    ~Q_AudioPlayer();

    void play(const std::filesystem::path& audioPath);
    void pause();
    void resume();
    void stop();

    [[nodiscard]] bool isPlaying() const;
    [[nodiscard]] bool isPaused() const;
    [[nodiscard]] bool isStopped() const;
    [[nodiscard]] QMediaPlayer::PlaybackState playbackState() const;

   signals:
    void started();
    void paused();
    void resumed();
    void stopped();
    void finished();  // EndOfMedia
    void errorOccurred(const QString& errorMessage);

   private slots:
    void _onMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void _onErrorOccurred(QMediaPlayer::Error error, const QString& errorMessage);

   private:
    static QString toQString(const std::filesystem::path& path);
    void _setupConnections();

    std::unique_ptr<QMediaPlayer> m_player;
    std::unique_ptr<QAudioOutput> m_audioOutput;
};