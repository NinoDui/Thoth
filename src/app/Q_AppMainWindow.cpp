#include "Q_AppMainWindow.h"

#include <QFileDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QStyle>

Q_AppMainWindow::Q_AppMainWindow(QWidget* parent) : QMainWindow(parent) {
    setupUI();
    setupConnections();

    setWindowTitle("Thoth");
    resize(800, 600);
}

void Q_AppMainWindow::setupUI() {
    QMenu* fileMenu = menuBar()->addMenu("File");
    fileMenu->addAction("Import File", this, &Q_AppMainWindow::onImportFile);
    fileMenu->addAction("Exit", this, &Q_AppMainWindow::close);

    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);
    QVBoxLayout* mainLayout = new QVBoxLayout(m_centralWidget);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    m_lblStatus = new QLabel("No file loaded", this);
    m_lblStatus->setAlignment(Qt::AlignCenter);

    m_lstContent = new QListWidget(this);
    m_lstContent->setFont(QFont("Arial", 12));
    m_lstContent->setSelectionMode(QAbstractItemView::SingleSelection);
    // m_lstContent->setStyleSheet("QListWidget { background-color: #f0f0f0; }");

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

    m_btnLoop = new QPushButton("Single Sentence", this);
    m_btnLoop->setCheckable(true);

    m_spinIndex = new QSpinBox(this);
    m_spinIndex->setRange(0, 1000000);
    m_spinIndex->setValue(0);
    m_spinIndex->setFixedWidth(60);

    m_btnJump = new QPushButton("Go", this);

    controlLayout->addWidget(m_btnPrev);
    controlLayout->addWidget(m_btnPlay);
    controlLayout->addWidget(m_btnPause);
    controlLayout->addStretch();
    controlLayout->addWidget(m_btnLoop);
    controlLayout->addWidget(m_spinIndex);
    controlLayout->addWidget(m_btnJump);

    mainLayout->addLayout(controlLayout);
}

void Q_AppMainWindow::setupConnections() {
    connect(m_btnPlay, &QPushButton::clicked, [this]() { m_audioManager->resume(); });
    connect(m_btnPause, &QPushButton::clicked, [this]() { m_audioManager->pause(); });
    connect(m_btnPrev, &QPushButton::clicked, [this]() { m_audioManager->playPrevious(); });

    connect(m_btnLoop, &QPushButton::toggled,
            [this](bool checked) { m_audioManager->setLoopSingle(checked); });
    connect(m_btnJump, &QPushButton::clicked,
            [this]() { m_audioManager->play(m_spinIndex->value()); });

    connect(m_lstContent, &QListWidget::itemDoubleClicked, [this](QListWidgetItem* item) {
        m_audioManager->play(item->data(Qt::UserRole).toInt());
    });
}

void Q_AppMainWindow::onImportFile() {
    QString fileName = QFileDialog::getOpenFileName(this, "Open Text", "", "Text Files (*.txt)");
    if (fileName.isEmpty()) return;

    m_lblStatus->setText("Loading: " + fileName);
    std::vector<std::string> sentences = m_textParser->parseFile(fileName.toStdString());

    m_lstContent->clear();
    for (const auto& sentence : sentences) {
        m_lstContent->addItem(QString::fromStdString(sentence));
    }
}

void Q_AppMainWindow::onExportAudio() {}

void Q_AppMainWindow::onPlayClicked() { m_audioManager->resume(); }

void Q_AppMainWindow::onPauseClicked() { m_audioManager->pause(); }

void Q_AppMainWindow::onPrevClicked() { m_audioManager->playPrevious(); }

void Q_AppMainWindow::onLoopToggled(bool checked) { m_audioManager->setLoopSingle(checked); }

void Q_AppMainWindow::onJumpToSentence(int idx) { m_audioManager->play(idx); }

void Q_AppMainWindow::onCoreStateChanged(QMediaPlayer::PlaybackState state) {
    m_btnPlay->setVisible(state == QMediaPlayer::StoppedState);
    m_btnPause->setVisible(state == QMediaPlayer::PlayingState);
}

void Q_AppMainWindow::onCoreSentenceChanged(int idx) {
    if (idx < 0 || idx >= m_lstContent->count()) return;

    m_lstContent->setCurrentRow(idx);
    m_lstContent->scrollToItem(m_lstContent->item(idx), QAbstractItemView::PositionAtCenter);
}
