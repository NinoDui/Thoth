#include "Q_AppMainWindow.h"

#include <QFileDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QStyle>

#include "Q_PlaybackControlBar.h"
#include "Q_SettingDialog.h"
#include "Q_ShadowingBar.h"
#include "thoth/ContentProvider.h"
#include "thoth/Entity.h"
#include "thoth/Logger.h"
#include "thoth/Q_PlaybackController.h"
#include "thoth/Q_RecordController.h"

Q_AppMainWindow::Q_AppMainWindow(QWidget* parent) : QMainWindow(parent) {
    setupControllers();
    setupUI();
    setupConnections();

    setWindowTitle("Thoth");
    resize(800, 600);
}

Q_AppMainWindow::~Q_AppMainWindow() = default;

void Q_AppMainWindow::setupControllers() {
    m_textContentProvider = std::make_unique<TextContentProvider>();
    m_playbackController = std::make_unique<Q_PlaybackController>(m_textContentProvider.get());
    m_recordController = std::make_unique<Q_RecordController>();
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

    m_playbackControlBar = new Q_PlaybackControlBar(this);
    mainLayout->addWidget(m_playbackControlBar);

    m_shadowingBar = new Q_ShadowingBar(this);
    mainLayout->addWidget(m_shadowingBar);
}

void Q_AppMainWindow::setupConnections() {
    // connections with the playbackController (Audio Play)
    connect(m_playbackControlBar, &Q_PlaybackControlBar::sigPlay,
            [this]() { m_playbackController->play(); });
    connect(m_playbackControlBar, &Q_PlaybackControlBar::sigPause,
            [this]() { m_playbackController->pause(); });
    connect(m_playbackControlBar, &Q_PlaybackControlBar::sigPrev,
            [this]() { m_playbackController->playPrevious(); });
    connect(m_playbackControlBar, &Q_PlaybackControlBar::sigNext,
            [this]() { m_playbackController->playNext(); });
    connect(m_playbackControlBar, &Q_PlaybackControlBar::sigLoopModeChanged, [this](bool checked) {
        m_playbackController->setLoopSingle(checked, m_playbackControlBar->loopDelay());
    });
    connect(m_playbackControlBar, &Q_PlaybackControlBar::sigDelayChanged, [this](int value) {
        m_playbackController->setLoopSingle(m_playbackControlBar->loopMode(), value);
    });
    connect(m_lstContent, &QListWidget::itemDoubleClicked, [this](QListWidgetItem* item) {
        m_playbackController->playSentence(item->data(Qt::UserRole).toInt());
    });
    connect(m_playbackController.get(), &Q_PlaybackController::playbackStarted, this,
            &Q_AppMainWindow::onCoreSentenceChanged);

    // connections with the shadowingBar (Shadowing Record)
    connect(m_shadowingBar, &Q_ShadowingBar::sigStartRecording, this, [this]() {
        int idx = m_lstContent->currentRow();
        if (idx < 0 || idx >= m_lstContent->count()) {
            LOG_WARN("Invalid index [{}], expected range [0, {})", idx, m_lstContent->count());
            return;
        }
        m_recordController->startRecording(m_currentSession.recordedSentences->at(idx).id);
    });
    connect(m_shadowingBar, &Q_ShadowingBar::sigStopRecording, this, [this]() {
        m_recordController->stopRecording();
        m_shadowingBar->onRecordingFinished();
    });
    connect(m_shadowingBar, &Q_ShadowingBar::sigPlayUserAudio, this,
            [this]() { LOG_CRITICAL("Play user audio is not implemented yet"); });
    connect(m_recordController.get(), &Q_RecordController::updateAmplitude, m_shadowingBar,
            &Q_ShadowingBar::setAmplitude);
    connect(m_recordController.get(), &Q_RecordController::errorOccurred, this,
            [this](const QString& errorMessage) {
                LOG_ERROR("Recorder error: {}", errorMessage.toStdString());
                m_shadowingBar->reset();
            });
}

void Q_AppMainWindow::onImportFile() {
    QString fileName = QFileDialog::getOpenFileName(this, "Open Text", "", "Text Files (*.txt)");
    if (fileName.isEmpty()) {
        LOG_WARN("No file selected, skipping ...");
        return;
    } else {
        LOG_INFO("File selected: {}", fileName.toStdString());
    }

    m_lblStatus->setText("Loading: " + fileName);
    m_textContentProvider->load(fileName.toStdString(), [this](Session session) {
        // A new session is created by the content provider
        // passed to this lambda callback via value copy
        // as the reference pass is not allowed in cross-thread async calls
        // QMetaObject::invokeMethod is packing the lambda function to a task (QMetaCallEvent) and
        // dispatch it to the main thread
        QMetaObject::invokeMethod(this, [this, session = std::move(session)]() {
            m_currentSession = std::move(session);
            m_currentSession.recordedSentences = std::vector<RecordedSentence>();
            for (const auto& sentence : m_currentSession.sentences) {
                m_currentSession.recordedSentences->push_back(
                    RecordedSentence{sentence, std::filesystem::path(), 0.0});
            }
            m_playbackController->setSession(m_currentSession);
            m_lblStatus->setText("Loaded " + QString::number(m_currentSession.sentences.size()) +
                                 " sentences");
            updateContentList();
            LOG_INFO("Loaded {} sentences", m_currentSession.sentences.size());
        });
    });
}

void Q_AppMainWindow::updateContentList() {
    m_lstContent->clear();
    for (size_t i = 0; i < m_currentSession.sentences.size(); ++i) {
        QString text = QString("[%1] %2").arg(i + 1).arg(
            QString::fromStdString(m_currentSession.sentences[i].text));
        QListWidgetItem* item = new QListWidgetItem(text);
        item->setData(Qt::UserRole, static_cast<int>(i));
        m_lstContent->addItem(item);
    }
}

void Q_AppMainWindow::onExportAudio() {}

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
