#pragma once

#include <fmt/ostream.h>

#include <QAudioOutput>
#include <QMediaPlayer>
#include <QObject>
#include <QTimer>
#include <memory>

#include "thoth/Entity.h"

class IContentProvider;

/**
 * @brief The Q_PlaybackController class is a Qt wrapper for the PlaybackController class.
 * It is used to control the playback of the content.
 */
class Q_PlaybackController : public QObject {
    Q_OBJECT
   public:
    explicit Q_PlaybackController(QObject* parent = nullptr);
    explicit Q_PlaybackController(IContentProvider* contentProvider, QObject* parent = nullptr);
    ~Q_PlaybackController();

    // load the data
    void setSession(const Session& session);

    // control the playback (business logic)
    void playSentence(int idx);
    void playNext();
    void playPrevious();

    void pause();
    void resume();
    void stop();
    void reset();

    void setLoopSingle(bool enable, int delaySeconds = 0);

    bool isPlaying() const;
    int getCurrentSentenceIndex() const;

    friend std::ostream& operator<<(std::ostream& os,
                                    const Q_PlaybackController& playbackController);
    friend QDebug operator<<(QDebug dbg, const Q_PlaybackController& playbackController);
    void dumpState() const;

   signals:
    // Signals to "communicate" with the UI
    void playbackStarted(int idx);
    void playbackPaused(int idx);
    void playbackStopped(int idx);
    // when a sentence is finished
    void playbackFinished(int idx);

    // Signals when the status is changed
    void statusMessageChanged(const QString& message);
    void errorOccurred(const QString& message);

   private slots:
    void _onMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void _onLoopTimerTimeout();
    void _onErrorOccurred(QMediaPlayer::Error error, const QString& errorMsg);

   private:
    void _Q_play(const std::filesystem::path& localPath);
    void _fetchNext(int start_idx);
    void setupConnections();

    std::unique_ptr<QMediaPlayer> m_player;
    std::unique_ptr<QAudioOutput> m_audioOutput;
    std::unique_ptr<QTimer> m_delayTimer;
    std::unique_ptr<IContentProvider> m_contentProvider;

    Session m_currentSession;
    int m_currentIdx = DEFAULT_START_IDX;
    bool m_singleLoop = false;
    int m_loopDelaySeconds = 0;
    int windows_size = PRELOAD_WINDOW;

    constexpr static int PRELOAD_WINDOW = 3;
    constexpr static int DEFAULT_START_IDX = 0;
};

template <>
struct fmt::formatter<Q_PlaybackController> : fmt::ostream_formatter {};