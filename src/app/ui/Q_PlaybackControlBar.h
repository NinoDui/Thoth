#pragma once

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QStyle>
#include <QWidget>

class Q_PlaybackControlBar : public QWidget {
    Q_OBJECT
   public:
    explicit Q_PlaybackControlBar(QWidget* parent = nullptr);

   signals:
    void sigPlay();
    void sigPause();
    void sigPrev();
    void sigNext();
    void sigLoopModeChanged(bool singleLoop);
    void sigDelayChanged(int delaySeconds);

   public slots:
    void setPlayingState(bool isPlaying);

   private:
    void setupUI();
    void setupConnections();

    QPushButton* m_btnPrev;
    QPushButton* m_btnPlayPause;
    QPushButton* m_btnNext;

    QCheckBox* m_chkLoop;
    QSpinBox* m_spinDelay;

    bool m_isPlaying = false;
};