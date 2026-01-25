#pragma once
#include <QObject>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <filesystem>
#include <vector>
#include <string>
#include <deque>
#include <set>

class AudioCache;
class TTSDownloader;

class AudioManager : public QObject {
    Q_OBJECT
public:
    explicit AudioManager(QObject* parent = nullptr);
    ~AudioManager();

    void setPlaylist(const std::vector<std::string>& sentences);
    void play(int idx);
    void pause();
    void resume();

    bool isPlaying() const;

signals:
    void currentSentenceChange(int idx);

private:
    void prepareAudio(int idx, bool isHighPriority);
    void checkPreload();
    void playLocalFile(const std::filesystem::path& path);

    std::unique_ptr<AudioCache> m_audioCache;
    TTSDownloader* m_ttsDownloader;

    QMediaPlayer* m_player;
    QAudioOutput* m_audioOutput;

    std::vector<std::string> m_playlist;
    int m_currentIdx = -1;
    // avoid repetitive downloads
    std::set<int> m_downloadingIdx;

    // Lazy load, preload with 3 sentences
    const int PRELOAD_WINDOW = 3;
};