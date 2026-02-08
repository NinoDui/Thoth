#pragma once

#include <QBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMainWindow>
#include <QMediaPlayer>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <memory>

#include "thoth/Entity.h"

class Q_PlaybackController;
class Q_PlaybackControlBar;
class TextContentProvider;
class Q_ShadowingBar;
class Q_RecordController;

class Q_AppMainWindow : public QMainWindow {
    Q_OBJECT

   public:
    explicit Q_AppMainWindow(QWidget* parent = nullptr);
    ~Q_AppMainWindow();

   private slots:
    void onImportFile();
    void onExportAudio();
    void onCoreSentenceChanged(int idx);

   private:
    void setupUI();
    void setupConnections();
    void setupControllers();

    void updateContentList();

    QWidget* m_centralWidget;
    QLabel* m_lblStatus;
    QListWidget* m_lstContent;
    Q_PlaybackControlBar* m_playbackControlBar;
    Q_ShadowingBar* m_shadowingBar;

    int m_lastHighlightedIdx = -1;

    std::unique_ptr<Q_PlaybackController> m_playbackController;
    std::unique_ptr<Q_RecordController> m_recordController;
    std::unique_ptr<TextContentProvider> m_textContentProvider;
    
    Session m_currentSession;
};