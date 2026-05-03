#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QStyle>
#include <QWidget>

class Q_PlaybackControlBar : public QWidget {
    Q_OBJECT
   public:
    explicit Q_PlaybackControlBar(QWidget* parent = nullptr);

    bool loopMode() const;
    int loopDelay() const;
    double playbackRate() const;

    void setPlaybackRate(double rate);
    void setDisplayMode(const QString& mode);

   signals:
    void sigPlay();
    void sigPause();
    void sigPrev();
    void sigNext();
    void sigLoopModeChanged(bool singleLoop);
    void sigDelayChanged(int delaySeconds);
    void sigRateChanged(double rate);
    void sigModeChanged(const QString& mode);

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

    QSlider* m_sliderRate;
    QLabel* m_lblRate;

    QComboBox* m_comboMode;

    bool m_isPlaying = false;
    bool m_singleLoop = false;
    int m_loopDelay = 0;
};
