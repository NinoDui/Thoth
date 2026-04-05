#include "Q_ShadowingBar.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>

Q_ShadowingBar::Q_ShadowingBar(QWidget* parent) : QWidget(parent) {
    setupUI();
    reset();
}

Q_ShadowingBar::~Q_ShadowingBar() = default;

void Q_ShadowingBar::reset() {
    m_isRecording = false;

    m_btnRecord->setEnabled(true);
    m_btnRecord->setProperty("recording", false);
    m_btnRecord->style()->unpolish(m_btnRecord);
    m_btnRecord->style()->polish(m_btnRecord);

    m_btnPlayUserAudio->setEnabled(false);
    m_visualizer->setValue(0);
    m_lblAnalyzing->setVisible(false);
    m_lblAnalysisResult->setVisible(false);
}

void Q_ShadowingBar::setupUI() {
    auto outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(10, 5, 10, 5);
    outerLayout->setSpacing(4);

    // Top row: record btn, visualizer, play btn
    auto topRow = new QHBoxLayout();
    topRow->setSpacing(10);

    m_btnRecord = new QPushButton(this);
    m_btnRecord->setObjectName("btnRecord");
    m_btnRecord->setFixedSize(40, 40);
    m_btnRecord->setCursor(Qt::PointingHandCursor);
    m_btnRecord->setToolTip("Hold or Click to Record");

    m_visualizer = new QProgressBar(this);
    m_visualizer->setObjectName("visualizer");
    m_visualizer->setRange(0, 100);
    m_visualizer->setValue(0);
    m_visualizer->setTextVisible(false);
    m_visualizer->setFixedHeight(6);

    m_btnPlayUserAudio = new QPushButton("Play my record", this);
    m_btnPlayUserAudio->setObjectName("btnPlayUserAudio");
    m_btnPlayUserAudio->setCursor(Qt::PointingHandCursor);
    m_btnPlayUserAudio->setEnabled(false);

    topRow->addWidget(m_btnRecord);
    topRow->addWidget(m_visualizer, 1);
    topRow->addWidget(m_btnPlayUserAudio);

    // Bottom row: status labels
    auto bottomRow = new QHBoxLayout();
    bottomRow->setSpacing(10);

    m_lblAnalyzing = new QLabel("Analyzing...", this);
    m_lblAnalyzing->setObjectName("lblAnalyzing");
    m_lblAnalyzing->setVisible(false);

    m_lblAnalysisResult = new QLabel(this);
    m_lblAnalysisResult->setObjectName("lblAnalysisResult");
    m_lblAnalysisResult->setVisible(false);

    bottomRow->addStretch();
    bottomRow->addWidget(m_lblAnalyzing);
    bottomRow->addWidget(m_lblAnalysisResult);

    outerLayout->addLayout(topRow);
    outerLayout->addLayout(bottomRow);

    // connections
    connect(m_btnRecord, &QPushButton::clicked, this, &Q_ShadowingBar::onRecordBtnClicked);
    connect(m_btnPlayUserAudio, &QPushButton::clicked, this, &Q_ShadowingBar::sigPlayUserAudio);
}

void Q_ShadowingBar::onRecordBtnClicked() {
    if (m_isRecording) {
        m_isRecording = false;
        m_btnRecord->setProperty("recording", false);
        m_btnRecord->style()->unpolish(m_btnRecord);
        m_btnRecord->style()->polish(m_btnRecord);
        emit sigStopRecording();
    } else {
        m_isRecording = true;
        m_btnRecord->setProperty("recording", true);
        m_btnRecord->style()->unpolish(m_btnRecord);
        m_btnRecord->style()->polish(m_btnRecord);

        m_btnPlayUserAudio->setEnabled(false);
        m_lblAnalysisResult->setVisible(false);
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

void Q_ShadowingBar::onASRAnalysisBusy(bool busy) {
    m_btnRecord->setEnabled(!busy);
    m_lblAnalyzing->setVisible(busy);
    if (busy) m_lblAnalysisResult->setVisible(false);
}

void Q_ShadowingBar::onASRAnalysisReady(double scorePercent) {
    m_lblAnalyzing->setVisible(false);
    m_lblAnalysisResult->setText(
        QString("Score: %1%").arg(static_cast<int>(scorePercent)));
    m_lblAnalysisResult->setVisible(true);
}
