#pragma once
#include <QAudioOutput>
#include <QMediaPlayer>
#include <QObject>
#include <QTimer>
#include <filesystem>
#include <memory>
#include <set>
#include <string>
#include <vector>

class AudioCache;
class TTSDownloader;

class AudioManager : public QObject {
    Q_OBJECT
   public:
    explicit AudioManager(QObject* parent = nullptr);
    ~AudioManager();

    void play(int idx);
    void pause();
    void resume();
    void stop();

    // Content
    void setPlaylist(const std::vector<std::string>& playlist);

    // Play Controlling
    bool isPlaying() const;
    void playNext();
    void playPrevious();

    // Mode Setting
    void setLoopSingle(bool enable, int delaySeconds = 0);

    // Export the audio file
    void exportAudioToOne(const std::filesystem::path& dstPath) const;

   signals:
    void currentSentenceChange(int idx);
    void playbackStarted(int idx);
    void playbackFinished(int idx);

   private:
    // Memory controlled by Qt
    QMediaPlayer* m_player;
    QAudioOutput* m_audioOutput;
    QTimer* m_delayTimer;

    TTSDownloader* m_ttsDownloader;
    std::unique_ptr<AudioCache> m_audioCache;

    // TODO: replace with reference to avoid deep copying
    std::vector<std::string> m_playlist;
    std::set<int> m_downloadingIdx;
    int m_curIdx = -1;
    bool m_singleLoop = false;
    int m_loopDelaySeconds = 0;

    // prepare the audio (visit the cache, download it if missed, then play it)
    void prepareAndPlay(int idx);

    // Lazy load, prefetch the audio files for the following context window.
    void fetchNext(int start_idx, int window_size = PRELOAD_WINDOW);

    static const int PRELOAD_WINDOW = 3;
};