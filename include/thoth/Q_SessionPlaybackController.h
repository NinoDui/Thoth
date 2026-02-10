#pragma once

#include <fmt/ostream.h>

#include <QObject>
#include <QTimer>
#include <memory>

#include "thoth/Entity.h"

class Q_AudioPlayer;
class IContentProvider;

class Q_SessionPlaybackController : public QObject {
    Q_OBJECT
   public:
    explicit Q_SessionPlaybackController(Q_AudioPlayer* audioPlayer,
                                         IContentProvider* contentProvider,
                                         QObject* parent = nullptr);
    ~Q_SessionPlaybackController();

    // Session lifecycle
    void setSession(const Session& session);
    void reset();

    // Playback control
    void play();
    void playSentence(int idx);
    void playNext();
    void playPrevious();
    void pause();
    void resume();
    void stop();

    // Loop control
    void setLoopSingle(bool enable, int delaySeconds = 0);

    // Status
    [[nodiscard]] int currentIndex() const;
    [[nodiscard]] bool isActive() const;  // this controller is "driving" the mediaplayer

    // debug and log
    friend std::ostream& operator<<(std::ostream& os,
                                    const Q_SessionPlaybackController& controller);
    friend QDebug operator<<(QDebug dbg, const Q_SessionPlaybackController& controller);
    void dumpState() const;

   signals:
    void sentencePlayStarted(int idx);
    void sentencePlayFinished(int idx);
    void sentencePlayPaused(int idx);
    void sentencePlayStopped(int idx);
    void statusMessageChanged(const QString& message);
    void errorOccurred(const QString& message);

   private slots:
    void _onPlayerFinished();
    void _onPlayerError(const QString& errorMessage);
    void _onLoopTimerTimeout();

   private:
    void _prepareThenPlay(int idx);
    void _prefetchNext(int startIdx);
    void _connectPlayer();
    void _disconnectPlayer();

    Q_AudioPlayer* m_audioPlayer;
    IContentProvider* m_contentProvider;

    // Session status
    Session m_session;
    int m_currentIdx = DEFAULT_START_IDX;
    bool m_active = false;

    // Loop orchestration
    std::unique_ptr<QTimer> m_loopTimer;
    bool m_singleLoop = false;
    int m_loopDelaySeconds = 0;

    static constexpr int DEFAULT_START_IDX = 0;
    static constexpr int PRELOAD_WINDOW = 3;
};

template <>
struct fmt::formatter<Q_SessionPlaybackController> : fmt::ostream_formatter {};