#pragma once

#include <QBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMainWindow>
#include <QMediaPlayer>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <memory>

#include "thoth/Entity.h"

class Q_SessionPlaybackController;
class Q_AudioPlayer;
class Q_PlaybackControlBar;
class TextContentProvider;
class AudioContentProvider;
class Q_ShadowingBar;
class Q_RecordController;
class Q_ASRController;
class WERScorer;

class Q_AppMainWindow : public QMainWindow {
    Q_OBJECT

   public:
    explicit Q_AppMainWindow(QWidget* parent = nullptr);
    ~Q_AppMainWindow();

   private slots:
    void onImportFile();
    void onImportAudio();
    void onExportAudio();
    void onCoreSentenceChanged(int idx);
    void onPlaybackError(const QString& errorMsg);
    void onConfigChanged(const QString& key);

   private:
    void setupUI();
    void setupControllers();
    void setupConnections();

    void setupPlaybackConnections();
    void setupRecordingConnections();
    void setupASRConnections();

    void recreateTTSEngine();
    void reloadWhisperConfig();

    void updateContentList();
    void openSettingsDialog();
    void restorePlaybackSettings();

    QWidget* m_centralWidget;
    QLabel* m_lblStatus;
    QListWidget* m_lstContent;
    QPlainTextEdit* m_txtInput;
    QPushButton* m_btnLoadText;
    QPushButton* m_btnLoadAudio;
    Q_PlaybackControlBar* m_playbackControlBar;
    Q_ShadowingBar* m_shadowingBar;

    int m_lastHighlightedIdx = -1;

    std::unique_ptr<TextContentProvider> m_textContentProvider;
    std::unique_ptr<AudioContentProvider> m_audioContentProvider;
    std::unique_ptr<Q_AudioPlayer> m_audioPlayer;
    std::unique_ptr<Q_SessionPlaybackController> m_sessionPlaybackController;
    std::unique_ptr<Q_RecordController> m_recordController;
    std::unique_ptr<WERScorer> m_werScorer;
    std::unique_ptr<Q_ASRController> m_asrController;

    Session m_currentSession;
};
