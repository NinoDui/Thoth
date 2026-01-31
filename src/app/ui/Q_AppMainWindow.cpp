#include "Q_AppMainWindow.h"

#include <QFileDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QStyle>

#include "Q_SettingDialog.h"
#include "thoth/Logger.h"

Q_AppMainWindow::Q_AppMainWindow(QWidget* parent)
    : QMainWindow(parent),
      m_audioManager(std::make_unique<Q_AudioManager>()),
      m_textParser(std::make_unique<TextParser>()) {
    setupUI();
    setupConnections();

    setWindowTitle("Thoth");
    resize(800, 600);
}

void Q_AppMainWindow::setupUI() {
    QMenu* fileMenu = menuBar()->addMenu("File");
    fileMenu->addAction("Import File", this, &Q_AppMainWindow::onImportFile);

    QAction* actSettings = fileMenu->addAction("Settings", this, [this]() {
        Q_SettingDialog dialog(this);
        if (dialog.exec() == QDialog::Accepted) {
            LOG_INFO("Settings updated by the user.");
        }
    });

    fileMenu->addSeparator();
    fileMenu->addAction("Exit", this, &Q_AppMainWindow::close);

    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);
    QVBoxLayout* mainLayout = new QVBoxLayout(m_centralWidget);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(15, 15, 15, 15);

    m_lblStatus = new QLabel("No file loaded", this);
    m_lblStatus->setAlignment(Qt::AlignCenter);

    m_lstContent = new QListWidget(this);
    m_lstContent->setObjectName("sentenceList");
    m_lstContent->setFont(QFont("Segoe UI", 12));
    m_lstContent->setWordWrap(true);
    m_lstContent->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_lstContent->setSelectionMode(QAbstractItemView::SingleSelection);

    mainLayout->addWidget(m_lblStatus);
    mainLayout->addWidget(m_lstContent, 1);

    // Control Buttons
    QHBoxLayout* controlLayout = new QHBoxLayout();
    m_btnPrev = new QPushButton("Prev", this);
    m_btnPrev->setIcon(style()->standardIcon(QStyle::SP_MediaSkipBackward));
    m_btnPrev->setToolTip("Previous Sentence");

    m_btnPlay = new QPushButton("Play", this);
    m_btnPlay->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));

    m_btnPause = new QPushButton("Pause", this);
    m_btnPause->setIcon(style()->standardIcon(QStyle::SP_MediaPause));

    m_btnNext = new QPushButton("Next", this);
    m_btnNext->setIcon(style()->standardIcon(QStyle::SP_MediaSkipForward));
    m_btnNext->setToolTip("Next Sentence");

    m_btnLoop = new QPushButton("Single Sentence", this);
    m_btnLoop->setCheckable(true);

    QLabel* lblDelay = new QLabel("Delay (seconds):", this);
    m_spinDelay = new QSpinBox(this);
    m_spinDelay->setRange(0, 5 * 60);
    m_spinDelay->setSuffix("s");
    m_spinDelay->setValue(0);
    m_spinDelay->setFixedWidth(60);
    m_spinDelay->setToolTip("Delay interval between sentences");

    m_spinIndex = new QSpinBox(this);
    m_spinIndex->setRange(0, 1000000);
    m_spinIndex->setValue(0);
    m_spinIndex->setFixedWidth(60);

    m_btnJump = new QPushButton("Go", this);

    controlLayout->addWidget(m_btnPrev);
    controlLayout->addWidget(m_btnPlay);
    controlLayout->addWidget(m_btnPause);
    controlLayout->addWidget(m_btnNext);
    controlLayout->addStretch();
    controlLayout->addWidget(lblDelay);
    controlLayout->addWidget(m_spinDelay);
    controlLayout->addWidget(m_btnLoop);
    controlLayout->addWidget(m_spinIndex);
    controlLayout->addWidget(m_btnJump);

    mainLayout->addLayout(controlLayout);
}

void Q_AppMainWindow::setupConnections() {
    connect(m_btnPlay, &QPushButton::clicked, [this]() { m_audioManager->resume(); });
    connect(m_btnPause, &QPushButton::clicked, [this]() { m_audioManager->pause(); });
    connect(m_btnPrev, &QPushButton::clicked, [this]() { m_audioManager->playPrevious(); });
    connect(m_btnNext, &QPushButton::clicked, [this]() { m_audioManager->playNext(); });
    connect(m_spinDelay, &QSpinBox::valueChanged,
            [this](int value) { m_audioManager->setLoopSingle(m_btnLoop->isChecked(), value); });

    connect(m_btnLoop, &QPushButton::toggled,
            [this](bool checked) { m_audioManager->setLoopSingle(checked); });
    connect(m_btnJump, &QPushButton::clicked,
            [this]() { m_audioManager->play(m_spinIndex->value()); });

    connect(m_lstContent, &QListWidget::itemDoubleClicked, [this](QListWidgetItem* item) {
        m_audioManager->play(item->data(Qt::UserRole).toInt());
    });

    connect(m_audioManager.get(), &Q_AudioManager::playbackStarted, this,
            &Q_AppMainWindow::onCoreSentenceChanged);
}

void Q_AppMainWindow::onImportFile() {
    QString fileName = QFileDialog::getOpenFileName(this, "Open Text", "", "Text Files (*.txt)");
    if (fileName.isEmpty()) {
        LOG_WARNING("No file selected, skipping ...");
        return;
    } else {
        LOG_INFO("File selected: {}", fileName.toStdString());
    }

    m_lblStatus->setText("Loading: " + fileName);
    std::vector<std::string> sentences = m_textParser->parseFile(fileName.toStdString());
    m_lblStatus->setText("Loaded " + QString::number(sentences.size()) + " sentences");
    LOG_DEBUG("Parsed {} sentences", sentences.size());

    m_lstContent->clear();
    m_lastHighlightedIdx = -1;
    m_audioManager->reset();

    for (size_t i = 0; i < sentences.size(); ++i) {
        QString text = QString("[%1] %2").arg(i + 1).arg(QString::fromStdString(sentences[i]));
        QListWidgetItem* item = new QListWidgetItem(text);
        item->setData(Qt::UserRole, (int)i);
        m_lstContent->addItem(item);
    }
    m_audioManager->setPlaylist(sentences);
}

void Q_AppMainWindow::onExportAudio() {}

void Q_AppMainWindow::onPlayClicked() {
    if (m_audioManager->isPlaying()) {
        m_audioManager->pause();
    } else {
        m_audioManager->resume();
    }
}

void Q_AppMainWindow::onPauseClicked() { m_audioManager->pause(); }

void Q_AppMainWindow::onPrevClicked() { m_audioManager->playPrevious(); }

void Q_AppMainWindow::onNextClicked() { m_audioManager->playNext(); }

void Q_AppMainWindow::onLoopToggled(bool checked) { m_audioManager->setLoopSingle(checked); }

void Q_AppMainWindow::onJumpToSentence(int idx) { m_audioManager->play(idx); }

void Q_AppMainWindow::onCoreStateChanged(QMediaPlayer::PlaybackState state) {
    m_btnPlay->setVisible(state == QMediaPlayer::StoppedState);
    m_btnPause->setVisible(state == QMediaPlayer::PlayingState);
}

void Q_AppMainWindow::onCoreSentenceChanged(int idx) {
    if (idx < 0 || idx >= m_lstContent->count()) return;

    if (m_lastHighlightedIdx >= 0 && m_lastHighlightedIdx < m_lstContent->count()) {
        QListWidgetItem* prevItem = m_lstContent->item(m_lastHighlightedIdx);
        QFont font = prevItem->font();
        font.setBold(false);
        prevItem->setFont(font);
    }

    QListWidgetItem* curItem = m_lstContent->item(idx);
    QFont font = curItem->font();
    font.setBold(true);
    curItem->setFont(font);
    m_lastHighlightedIdx = idx;

    m_lstContent->setCurrentRow(idx);
    m_lstContent->scrollToItem(m_lstContent->item(idx), QAbstractItemView::PositionAtCenter);
}
