#include "Q_PlaybackControlBar.h"

#include <algorithm>

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
    QLabel* lblDelay = new QLabel("Delay (s):", this);
    m_spinDelay = new QSpinBox(this);
    m_spinDelay->setRange(0, 5 * 60);
    m_spinDelay->setSuffix("s");
    m_spinDelay->setValue(0);
    m_spinDelay->setFixedWidth(60);
    m_spinDelay->setToolTip("Delay interval between sentences");

    // [rate] slider
    m_lblRate = new QLabel("1.0x", this);
    m_lblRate->setFixedWidth(40);
    m_lblRate->setAlignment(Qt::AlignCenter);
    m_sliderRate = new QSlider(Qt::Horizontal, this);
    m_sliderRate->setRange(50, 200);  // 0.5x to 2.0x
    m_sliderRate->setValue(100);
    m_sliderRate->setFixedWidth(100);
    m_sliderRate->setToolTip("Playback speed");

    // [mode] combo
    m_comboMode = new QComboBox(this);
    m_comboMode->addItems({"Normal", "Pause-and-Repeat", "Simultaneous"});
    m_comboMode->setToolTip("Shadowing mode");

    // Layout
    layout->addWidget(m_btnPrev);
    layout->addWidget(m_btnPlayPause);
    layout->addWidget(m_btnNext);

    layout->addStretch();

    layout->addWidget(m_lblRate);
    layout->addWidget(m_sliderRate);

    layout->addWidget(m_comboMode);

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
    connect(m_sliderRate, &QSlider::valueChanged, [this](int value) {
        double rate = value / 100.0;
        m_lblRate->setText(QString("%1x").arg(rate, 0, 'f', 1));
        emit sigRateChanged(rate);
    });
    connect(m_comboMode, &QComboBox::currentTextChanged, this,
            &Q_PlaybackControlBar::sigModeChanged);
}

void Q_PlaybackControlBar::setPlayingState(bool isPlaying) {
    m_isPlaying = isPlaying;
    m_btnPlayPause->setIcon(
        style()->standardIcon(m_isPlaying ? QStyle::SP_MediaPause : QStyle::SP_MediaPlay));
}

bool Q_PlaybackControlBar::loopMode() const { return m_singleLoop; }
int Q_PlaybackControlBar::loopDelay() const { return m_loopDelay; }
double Q_PlaybackControlBar::playbackRate() const { return m_sliderRate->value() / 100.0; }

void Q_PlaybackControlBar::setPlaybackRate(double rate) {
    int val = static_cast<int>(rate * 100.0);
    val = std::clamp(val, m_sliderRate->minimum(), m_sliderRate->maximum());
    m_sliderRate->setValue(val);
    m_lblRate->setText(QString("%1x").arg(val / 100.0, 0, 'f', 1));
}

void Q_PlaybackControlBar::setDisplayMode(const QString& mode) {
    m_comboMode->setCurrentText(mode);
}
