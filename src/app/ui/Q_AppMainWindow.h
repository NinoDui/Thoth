#pragma once

#include <QBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMainWindow>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <memory>

#include "thoth/Q_AudioManager.h"
#include "thoth/TextParser.h"

class Q_AppMainWindow : public QMainWindow {
    Q_OBJECT

   public:
    explicit Q_AppMainWindow(QWidget* parent = nullptr);
    ~Q_AppMainWindow() = default;

   private slots:
    void onImportFile();
    void onExportAudio();

    void onPlayClicked();
    void onPauseClicked();
    void onPrevClicked();
    void onNextClicked();
    void onLoopToggled(bool checked);
    void onJumpToSentence(int idx);

    void onCoreSentenceChanged(int idx);
    void onCoreStateChanged(QMediaPlayer::PlaybackState state);

   private:
    void setupUI();
    void setupConnections();

    QWidget* m_centralWidget;

    QLabel* m_lblStatus;
    QListWidget* m_lstContent;

    QPushButton* m_btnPlay;
    QPushButton* m_btnPrev;
    QPushButton* m_btnPause;
    QPushButton* m_btnNext;
    QPushButton* m_btnLoop;

    QSpinBox* m_spinIndex;
    QPushButton* m_btnJump;

    QSpinBox* m_spinDelay;

    int m_lastHighlightedIdx = -1;

    std::unique_ptr<Q_AudioManager> m_audioManager;
    std::unique_ptr<TextParser> m_textParser;
};