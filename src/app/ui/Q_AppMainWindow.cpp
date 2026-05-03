#include "Q_AppMainWindow.h"

#include <QAudioFormat>
#include <QCoreApplication>
#include <QFileDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QStyle>
#include <QTextStream>

#include "HtmlDelegate.h"
#include "Q_PlaybackControlBar.h"
#include "Q_SettingDialog.h"
#include "Q_ShadowingBar.h"
#include "thoth/ConfigKey.h"
#include "thoth/ConfigStore.h"
#include "thoth/ContentProvider.h"
#include "thoth/Entity.h"
#include "thoth/Logger.h"
#include "thoth/Q_ASRController.h"
#include "thoth/Q_AudioPlayer.h"
#include "thoth/Q_RecordController.h"
#include "thoth/Q_SessionPlaybackController.h"
#include "thoth/TTSEngineFactory.h"
#include "thoth/WERScorer.h"

Q_AppMainWindow::Q_AppMainWindow(QWidget* parent) : QMainWindow(parent) {
    setupControllers();
    setupUI();
    setupConnections();

    connect(&ConfigStore::instance(), &ConfigStore::configChanged, this,
            &Q_AppMainWindow::onConfigChanged);

    restorePlaybackSettings();

    setWindowTitle("Thoth");
    resize(800, 600);
}

Q_AppMainWindow::~Q_AppMainWindow() = default;

void Q_AppMainWindow::setupControllers() {
    auto& config = ConfigStore::instance();
    auto whisperConfig = config.getWhisperConfig();
    auto audioConfig = config.getAudioRecorderConfig();

    auto ttsEngine =
        thoth::CreateTTSEngine(config.getTTSEngineName(), "", config.getPiperModelPath().string());

    auto cacheDir = config.getCacheDir() / "audio" / config.getTTSEngineName();
    m_textContentProvider = std::make_unique<TextContentProvider>(ttsEngine, cacheDir);
    m_audioPlayer = std::make_unique<Q_AudioPlayer>();

    m_sessionPlaybackController = std::make_unique<Q_SessionPlaybackController>(
        m_audioPlayer.get(), m_textContentProvider.get());

    QAudioFormat recordFormat;
    recordFormat.setSampleRate(audioConfig.sampleRate);
    recordFormat.setChannelCount(audioConfig.channels);
    recordFormat.setSampleFormat([](uint16_t bits) {
        switch (bits) {
            case 8:
                return QAudioFormat::UInt8;
            case 32:
                return QAudioFormat::Int32;
            case 64:
                return QAudioFormat::Float;
            default:
                return QAudioFormat::Int16;
        }
    }(audioConfig.sampleFormatBits));

    m_recordController = std::make_unique<Q_RecordController>(recordFormat, audioConfig.rmsStep);

    m_werScorer = std::make_unique<WERScorer>();
    m_asrController = std::make_unique<Q_ASRController>(m_werScorer.get(), whisperConfig);
}

void Q_AppMainWindow::setupUI() {
    QMenu* fileMenu = menuBar()->addMenu("File");
    fileMenu->addAction("Import File", this, &Q_AppMainWindow::onImportFile);

    QAction* actSettings =
        fileMenu->addAction("Settings", this, [this]() { openSettingsDialog(); });

    fileMenu->addSeparator();
    fileMenu->addAction("Exit", this, &Q_AppMainWindow::close);

    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);
    QVBoxLayout* mainLayout = new QVBoxLayout(m_centralWidget);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(15, 15, 15, 15);

    m_lblStatus = new QLabel("No file loaded", this);
    m_lblStatus->setAlignment(Qt::AlignCenter);

    m_txtInput = new QPlainTextEdit(this);
    m_txtInput->setPlaceholderText("Paste text here for shadowing practice...");
    m_txtInput->setMaximumHeight(100);

    m_btnLoadText = new QPushButton("Load Text", this);
    m_btnLoadText->setFixedWidth(100);

    auto* inputLayout = new QHBoxLayout();
    inputLayout->addWidget(m_txtInput, 1);
    inputLayout->addWidget(m_btnLoadText);

    m_lstContent = new QListWidget(this);
    m_lstContent->setObjectName("sentenceList");
    m_lstContent->setFont(QFont("Segoe UI", 12));
    m_lstContent->setWordWrap(true);
    m_lstContent->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_lstContent->setSelectionMode(QAbstractItemView::SingleSelection);
    m_lstContent->setItemDelegate(new HtmlDelegate(m_lstContent));

    mainLayout->addWidget(m_lblStatus);
    mainLayout->addLayout(inputLayout);
    mainLayout->addWidget(m_lstContent, 1);

    m_playbackControlBar = new Q_PlaybackControlBar(this);
    mainLayout->addWidget(m_playbackControlBar);

    m_shadowingBar = new Q_ShadowingBar(this);
    mainLayout->addWidget(m_shadowingBar);
}

void Q_AppMainWindow::setupConnections() {
    connect(m_btnLoadText, &QPushButton::clicked, [this]() {
        QString text = m_txtInput->toPlainText().trimmed();
        if (text.isEmpty()) {
            m_lblStatus->setText("No text entered");
            return;
        }
        m_lblStatus->setText("Loading text...");
        m_textContentProvider->loadFromText(text.toStdString(), [this](Session session) {
            QMetaObject::invokeMethod(this, [this, session = std::move(session)]() {
                m_currentSession = std::move(session);
                m_currentSession.recordedSentences = std::vector<RecordedSentence>();
                for (const auto& sentence : m_currentSession.sentences) {
                    m_currentSession.recordedSentences->push_back(RecordedSentence{
                        sentence, std::filesystem::path(), 0.0, std::nullopt, std::nullopt});
                }
                m_sessionPlaybackController->setSession(m_currentSession);
                m_lblStatus->setText(
                    "Loaded " + QString::number(m_currentSession.sentences.size()) + " sentences");
                updateContentList();
                LOG_INFO("Loaded {} sentences from text input", m_currentSession.sentences.size());
            });
        });
    });

    setupPlaybackConnections();
    setupRecordingConnections();
    setupASRConnections();
}

void Q_AppMainWindow::setupPlaybackConnections() {
    connect(m_playbackControlBar, &Q_PlaybackControlBar::sigPlay, m_sessionPlaybackController.get(),
            &Q_SessionPlaybackController::play);
    connect(m_playbackControlBar, &Q_PlaybackControlBar::sigPause,
            m_sessionPlaybackController.get(), &Q_SessionPlaybackController::pause);
    connect(m_playbackControlBar, &Q_PlaybackControlBar::sigPrev, m_sessionPlaybackController.get(),
            &Q_SessionPlaybackController::playPrevious);
    connect(m_playbackControlBar, &Q_PlaybackControlBar::sigNext, m_sessionPlaybackController.get(),
            &Q_SessionPlaybackController::playNext);
    connect(m_playbackControlBar, &Q_PlaybackControlBar::sigLoopModeChanged, [this](bool checked) {
        m_sessionPlaybackController->setLoopSingle(checked, m_playbackControlBar->loopDelay());
    });
    connect(m_playbackControlBar, &Q_PlaybackControlBar::sigDelayChanged, [this](int value) {
        m_sessionPlaybackController->setLoopSingle(m_playbackControlBar->loopMode(), value);
    });
    connect(m_playbackControlBar, &Q_PlaybackControlBar::sigRateChanged, [this](double rate) {
        m_audioPlayer->setRate(rate);
        ConfigStore::instance().setValue(thoth::config::KEY_PLAYBACK_RATE, rate);
    });
    connect(m_playbackControlBar, &Q_PlaybackControlBar::sigModeChanged,
            [this](const QString& mode) {
                std::string modeStr;
                if (mode == "Pause-and-Repeat") {
                    modeStr = "pause-and-repeat";
                } else if (mode == "Simultaneous") {
                    modeStr = "simultaneous";
                } else {
                    modeStr = "normal";
                }
                m_sessionPlaybackController->setMode(modeStr);
                ConfigStore::instance().setValue(thoth::config::KEY_PLAYBACK_MODE, modeStr);
            });
    connect(m_lstContent, &QListWidget::itemDoubleClicked, [this](QListWidgetItem* item) {
        m_sessionPlaybackController->playSentence(item->data(Qt::UserRole).toInt());
    });
    connect(m_sessionPlaybackController.get(), &Q_SessionPlaybackController::sentencePlayStarted,
            this, &Q_AppMainWindow::onCoreSentenceChanged);
    connect(m_sessionPlaybackController.get(), &Q_SessionPlaybackController::sentencePlayStarted,
            this, [this](int) { m_playbackControlBar->setPlayingState(true); });
    connect(m_sessionPlaybackController.get(), &Q_SessionPlaybackController::sentencePlayStopped,
            this, [this](int) { m_playbackControlBar->setPlayingState(false); });
    connect(m_sessionPlaybackController.get(), &Q_SessionPlaybackController::sentencePlayPaused,
            this, [this](int) { m_playbackControlBar->setPlayingState(false); });
    connect(m_sessionPlaybackController.get(), &Q_SessionPlaybackController::errorOccurred, this,
            &Q_AppMainWindow::onPlaybackError);
}

void Q_AppMainWindow::setupRecordingConnections() {
    connect(m_shadowingBar, &Q_ShadowingBar::sigStartRecording, this, [this]() {
        bool simultaneous = m_sessionPlaybackController->mode() == "simultaneous";
        if (!simultaneous) {
            m_sessionPlaybackController->stop();
            m_playbackControlBar->setPlayingState(false);
        }

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
    connect(m_shadowingBar, &Q_ShadowingBar::sigPlayUserAudio, this, [this]() {
        int idx = m_lstContent->currentRow();
        if (idx < 0 || idx >= m_lstContent->count()) {
            LOG_WARN("Invalid index [{}], expected range [0, {})", idx, m_lstContent->count());
            return;
        }
        const auto& path = m_currentSession.recordedSentences->at(idx).localShadowingPath;
        if (path.empty() || !std::filesystem::exists(path)) {
            LOG_WARN("No shadowing audio found for sentence [{}]", idx);
            return;
        }

        m_sessionPlaybackController->stop();
        m_sessionPlaybackController->suspendAutoAdvance();
        m_playbackControlBar->setPlayingState(false);
        m_audioPlayer->play(path);
    });
    connect(m_recordController.get(), &Q_RecordController::updateAmplitude, m_shadowingBar,
            &Q_ShadowingBar::setAmplitude);
    connect(m_recordController.get(), &Q_RecordController::errorOccurred, this,
            [this](const QString& errorMessage) {
                LOG_ERROR("Recorder error: {}", errorMessage.toStdString());
                m_shadowingBar->reset();
            });
    connect(m_recordController.get(), &Q_RecordController::recordingFinished, this,
            [this](const std::filesystem::path& path) {
                int idx = m_lstContent->currentRow();
                if (idx < 0 || idx >= m_lstContent->count()) {
                    LOG_WARN("Invalid index [{}], expected range [0, {})", idx,
                             m_lstContent->count());
                    return;
                }
                auto& rs = m_currentSession.recordedSentences->at(idx);
                rs.localShadowingPath = path;
                m_asrController->analyze(&rs);
            });

    connect(m_sessionPlaybackController.get(), &Q_SessionPlaybackController::repeatRequested,
            [this](int idx) {
                m_lstContent->setCurrentRow(idx);
                m_shadowingBar->triggerRecording();
            });
}

void Q_AppMainWindow::setupASRConnections() {
    connect(m_asrController.get(), &Q_ASRController::busyChanged, m_shadowingBar,
            &Q_ShadowingBar::onASRAnalysisBusy);
    connect(
        m_asrController.get(), &Q_ASRController::analysisReady, this, [this](RecordedSentence* rs) {
            double scorePercent = rs->shadowingScore * 100.0;
            m_shadowingBar->onASRAnalysisReady(scorePercent);

            auto* sentencesVec = &m_currentSession.recordedSentences.value();
            int idx = static_cast<int>(rs - sentencesVec->data());
            if (idx >= 0 && idx < m_lstContent->count()) {
                QString badge = QString("[%1%] ").arg(static_cast<int>(scorePercent));
                QString text = QString("[%1] %2").arg(idx + 1).arg(
                    QString::fromStdString(m_currentSession.sentences[idx].text));

                if (rs->scoringDetail && !rs->scoringDetail->alignedTokens.empty()) {
                    QString richText;
                    QTextStream stream(&richText);
                    stream
                        << QString("[%1] [%2%] ").arg(idx + 1).arg(static_cast<int>(scorePercent));
                    for (const auto& t : rs->scoringDetail->alignedTokens) {
                        QString color;
                        switch (t.label) {
                            case TokenLabel::Correct:
                                color = "#2e7d32";
                                break;
                            case TokenLabel::Missing:
                                color = "#c62828";
                                break;
                            case TokenLabel::Different:
                                color = "#1565c0";
                                break;
                            default:
                                break;
                        }
                        stream << "<span style='color:" << color << ";'>"
                               << QString::fromStdString(t.token).toHtmlEscaped() << "</span> ";
                    }
                    if (!rs->scoringDetail->extraTokens.empty()) {
                        stream << "<span style='color:#888888;'>(+";
                        for (size_t e = 0; e < rs->scoringDetail->extraTokens.size(); ++e) {
                            if (e > 0) stream << " ";
                            stream << QString::fromStdString(rs->scoringDetail->extraTokens[e])
                                          .toHtmlEscaped();
                        }
                        stream << ")</span>";
                    }
                    m_lstContent->item(idx)->setText(richText);
                } else {
                    m_lstContent->item(idx)->setText(
                        QString("[%1] [%2%] %3")
                            .arg(idx + 1)
                            .arg(static_cast<int>(scorePercent))
                            .arg(QString::fromStdString(m_currentSession.sentences[idx].text)));
                }
            }
        });
    connect(m_asrController.get(), &Q_ASRController::errorOccurred, this,
            [](const QString& msg) { LOG_ERROR("ASR error: {}", msg.toStdString()); });
}

void Q_AppMainWindow::onConfigChanged(const QString& key) {
    using namespace thoth::config;
    std::string k = key.toStdString();

    if (k == KEY_TTS_ENGINE || k == KEY_TTS_VOICE || k == KEY_TTS_LANG ||
        k == KEY_TTS_AUDIO_ENCODING || k == KEY_TTS_PIPER_MODEL_PATH) {
        recreateTTSEngine();
        return;
    }

    if (k == KEY_WHISPER_MODEL_PATH || k == KEY_WHISPER_MODEL_LANGUAGE) {
        reloadWhisperConfig();
        return;
    }

    if (k == KEY_PLAYBACK_RATE) {
        auto rate = ConfigStore::instance().getValue<double>(KEY_PLAYBACK_RATE);
        if (rate && *rate > 0.0) {
            m_audioPlayer->setRate(*rate);
        }
        return;
    }

    if (k == KEY_PROXY) {
        auto proxy = ConfigStore::instance().getValue<std::string>(KEY_PROXY);
        if (proxy && !proxy->empty()) {
            QByteArray p = proxy->c_str();
            qputenv("http_proxy", p);
            qputenv("https_proxy", p);
            qputenv("grpc_proxy", p);
        }
        return;
    }
}

void Q_AppMainWindow::recreateTTSEngine() {
    auto& config = ConfigStore::instance();
    auto engine =
        thoth::CreateTTSEngine(config.getTTSEngineName(), "", config.getPiperModelPath().string());

    auto cacheDir = config.getCacheDir() / "audio" / config.getTTSEngineName();
    m_textContentProvider = std::make_unique<TextContentProvider>(std::move(engine), cacheDir);

    m_sessionPlaybackController->stop();
    m_sessionPlaybackController = std::make_unique<Q_SessionPlaybackController>(
        m_audioPlayer.get(), m_textContentProvider.get());

    if (!m_currentSession.sentences.empty()) {
        m_sessionPlaybackController->setSession(m_currentSession);
    }

    setupPlaybackConnections();

    LOG_INFO("TTS engine recreated live");
}

void Q_AppMainWindow::reloadWhisperConfig() {
    auto& config = ConfigStore::instance();
    m_asrController->reloadModel(config.getWhisperConfig());
    LOG_INFO("Whisper config reloaded live");
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
                m_currentSession.recordedSentences->push_back(RecordedSentence{
                    sentence, std::filesystem::path(), 0.0, std::nullopt, std::nullopt});
            }
            m_sessionPlaybackController->setSession(m_currentSession);
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

void Q_AppMainWindow::onPlaybackError(const QString& errorMsg) {
    LOG_ERROR("Playback error: {}", errorMsg.toStdString());

    QMessageBox msgBox(this);
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setWindowTitle("Audio Playback Error");
    msgBox.setText("Failed to download audio from Google Cloud TTS.");
    msgBox.setInformativeText(
        "This may be caused by network issues or missing proxy configuration.\n\n"
        "You can configure your proxy in Settings.\n\n"
        "Details: " +
        errorMsg);
    QPushButton* openSettingsBtn = msgBox.addButton("Open Settings", QMessageBox::ActionRole);
    msgBox.addButton(QMessageBox::Ok);
    msgBox.exec();

    if (msgBox.clickedButton() == openSettingsBtn) {
        openSettingsDialog();
    }
}

void Q_AppMainWindow::onExportAudio() {}

void Q_AppMainWindow::openSettingsDialog() {
    Q_SettingDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) return;

    LOG_INFO("Settings applied");
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

void Q_AppMainWindow::restorePlaybackSettings() {
    auto& config = ConfigStore::instance();
    auto rate = config.getValue<double>(thoth::config::KEY_PLAYBACK_RATE);
    if (rate && *rate > 0.0) {
        m_playbackControlBar->setPlaybackRate(*rate);
        m_audioPlayer->setRate(*rate);
    }

    auto mode = config.getValue<std::string>(thoth::config::KEY_PLAYBACK_MODE);
    if (mode) {
        std::string m = *mode;
        if (m == "pause-and-repeat") {
            m_playbackControlBar->setDisplayMode("Pause-and-Repeat");
        } else if (m == "simultaneous") {
            m_playbackControlBar->setDisplayMode("Simultaneous");
        } else {
            m_playbackControlBar->setDisplayMode("Normal");
        }
        m_sessionPlaybackController->setMode(m);
    }
}
