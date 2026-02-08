#include "Q_ShadowingBar.h"

#include <QHBoxLayout>
#include <QProgressBar>
#include <QPushButton>
#include <QStyle>

Q_ShadowingBar::Q_ShadowingBar(QWidget* parent) : QWidget(parent) {
    setupUI();
    reset();
}

Q_ShadowingBar::~Q_ShadowingBar() = default;

void Q_ShadowingBar::reset() {
    m_isRecording = false;

    m_btnRecord->setProperty("recording", false);
    m_btnRecord->style()->unpolish(m_btnRecord);
    m_btnRecord->style()->polish(m_btnRecord);

    m_btnPlayUserAudio->setEnabled(false);
    m_visualizer->setValue(0);
}

void Q_ShadowingBar::setupUI() {
    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 5, 10, 5);
    layout->setSpacing(10);

    // Record Button
    m_btnRecord = new QPushButton(this);
    m_btnRecord->setObjectName("btnRecord");
    m_btnRecord->setFixedSize(40, 40);
    m_btnRecord->setCursor(Qt::PointingHandCursor);
    m_btnRecord->setToolTip("Hold or Click to Record");

    // Visualizer
    m_visualizer = new QProgressBar(this);
    m_visualizer->setObjectName("visualizer");
    m_visualizer->setRange(0, 100);
    m_visualizer->setValue(0);
    m_visualizer->setTextVisible(false);
    m_visualizer->setFixedHeight(6);

    // Playback Button
    m_btnPlayUserAudio = new QPushButton("Play my record", this);
    m_btnPlayUserAudio->setObjectName("btnPlayUserAudio");
    m_btnPlayUserAudio->setCursor(Qt::PointingHandCursor);
    m_btnPlayUserAudio->setEnabled(false);

    // Layout
    layout->addWidget(m_btnRecord);
    layout->addWidget(m_visualizer, 1);
    layout->addWidget(m_btnPlayUserAudio);

    // connections
    connect(m_btnRecord, &QPushButton::clicked, this, &Q_ShadowingBar::onRecordBtnClicked);
    connect(m_btnPlayUserAudio, &QPushButton::clicked, this, &Q_ShadowingBar::sigPlayUserAudio);
}

void Q_ShadowingBar::onRecordBtnClicked() {
    if (m_isRecording) {
        // state: recording -> stop
        m_isRecording = false;
        m_btnRecord->setProperty("recording", false);
        m_btnRecord->style()->unpolish(m_btnRecord);
        m_btnRecord->style()->polish(m_btnRecord);
        emit sigStopRecording();
    } else {
        // state: stop -> recording
        m_isRecording = true;
        m_btnRecord->setProperty("recording", true);
        m_btnRecord->style()->unpolish(m_btnRecord);
        m_btnRecord->style()->polish(m_btnRecord);

        m_btnPlayUserAudio->setEnabled(false);
        emit sigStartRecording();
    }
}

void Q_ShadowingBar::setAmplitude(float amplitude) {
    int val = static_cast<int>(amplitude * 100);
    m_visualizer->setValue(val);
}

void Q_ShadowingBar::onRecordingFinished() {
    m_isRecording = false;
    m_btnRecord->setProperty("recording", false);
    m_btnRecord->style()->unpolish(m_btnRecord);
    m_btnRecord->style()->polish(m_btnRecord);
    m_btnPlayUserAudio->setEnabled(true);
}