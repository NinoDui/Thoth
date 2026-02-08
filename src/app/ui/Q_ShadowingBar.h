#pragma once

#include <QWidget>

class QPushButton;
class QProgressBar;
class QHBoxLayout;
class Q_RecordController;

class Q_ShadowingBar : public QWidget {
    Q_OBJECT
   public:
    explicit Q_ShadowingBar(QWidget* parent = nullptr);
    ~Q_ShadowingBar();

    void reset();

   public slots:
    // called by core
    void setAmplitude(float amplitude);
    void onRecordingFinished();

   signals:
    // signals to the parent widget, Q_AppMainWindow
    void sigStartRecording();
    void sigStopRecording();
    void sigPlayUserAudio();

   private slots:
    // internal state changes
    void onRecordBtnClicked();

   private:
    void setupUI();
    void updateStyle();

    QPushButton* m_btnRecord;
    QPushButton* m_btnPlayUserAudio;
    QProgressBar* m_visualizer;

    bool m_isRecording = false;
};