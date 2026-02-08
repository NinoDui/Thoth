#include "Q_PlaybackControlBar.h"

Q_PlaybackControlBar::Q_PlaybackControlBar(QWidget* parent) : QWidget(parent) {
    setupUI();
    setupConnections();

    m_isPlaying = false;
}

void Q_PlaybackControlBar::setupUI() {
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 5, 10, 5);

    // [A] Prev
    m_btnPrev = new QPushButton(this);
    m_btnPrev->setIcon(style()->standardIcon(QStyle::SP_MediaSkipBackward));
    m_btnPrev->setToolTip("Previous Sentence");

    // [B] Play/Pause
    m_btnPlayPause = new QPushButton(this);
    m_btnPlayPause->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    m_btnPlayPause->setToolTip("Play/Pause");

    // [C] Next
    m_btnNext = new QPushButton(this);
    m_btnNext->setIcon(style()->standardIcon(QStyle::SP_MediaSkipForward));
    m_btnNext->setToolTip("Next Sentence");

    // [single] checkbox
    m_chkLoop = new QCheckBox("SingleLoop", this);

    // [delay] spinbox
    QLabel* lblDelay = new QLabel("Delay (seconds):", this);
    m_spinDelay = new QSpinBox(this);
    m_spinDelay->setRange(0, 5 * 60);
    m_spinDelay->setSuffix("s");
    m_spinDelay->setValue(0);
    m_spinDelay->setFixedWidth(60);
    m_spinDelay->setToolTip("Delay interval between sentences");

    // Layout
    layout->addWidget(m_btnPrev);
    layout->addWidget(m_btnPlayPause);
    layout->addWidget(m_btnNext);

    layout->addStretch();

    layout->addWidget(m_chkLoop);
    layout->addWidget(lblDelay);
    layout->addWidget(m_spinDelay);
}

void Q_PlaybackControlBar::setupConnections() {
    connect(m_btnPrev, &QPushButton::clicked, this, &Q_PlaybackControlBar::sigPrev);
    connect(m_btnNext, &QPushButton::clicked, this, &Q_PlaybackControlBar::sigNext);

    connect(m_btnPlayPause, &QPushButton::clicked, [this]() {
        if (m_isPlaying) {
            emit sigPause();
        } else {
            emit sigPlay();
        }
        setPlayingState(!m_isPlaying);
    });
    connect(m_chkLoop, &QCheckBox::toggled, [this](bool checked) {
        m_singleLoop = checked;
        emit sigLoopModeChanged(checked);
    });
    connect(m_spinDelay, &QSpinBox::valueChanged, [this](int value) {
        m_loopDelay = value;
        emit sigDelayChanged(value);
    });
}

void Q_PlaybackControlBar::setPlayingState(bool isPlaying) {
    m_isPlaying = isPlaying;
    m_btnPlayPause->setIcon(
        style()->standardIcon(m_isPlaying ? QStyle::SP_MediaPause : QStyle::SP_MediaPlay));
}

bool Q_PlaybackControlBar::loopMode() const { return m_singleLoop; }
int Q_PlaybackControlBar::loopDelay() const { return m_loopDelay; }