#include "Q_SettingDialog.h"

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

#include "thoth/ConfigKey.h"
#include "thoth/ConfigStore.h"
#include "thoth/Logger.h"
#include "thoth/Q_GCPVoiceLoader.h"

namespace {
constexpr const char* kTtsLanguages[] = {"English (en-US)", "Swedish (sv-SE)", "Japanese (ja-JP)",
                                         "Korean (ko-KR)"};

std::string languageCodeFromDisplay(const QString& display) {
    if (display.startsWith("English")) return "en-US";
    if (display.startsWith("Swedish")) return "sv-SE";
    if (display.startsWith("Japanese")) return "ja-JP";
    if (display.startsWith("Korean")) return "ko-KR";
    return "en-US";
}
}  // namespace

Q_SettingDialog::Q_SettingDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Settings");
    resize(400, 300);
    setupUI();
    loadSettings();
}

Q_SettingDialog::~Q_SettingDialog() = default;

void Q_SettingDialog::setupUI() {
    auto mainLayout = new QVBoxLayout(this);
    m_tabWidget = new QTabWidget(this);

    // Tab [General]
    QWidget* generalTab = new QWidget(this);
    auto* generalLayout = new QVBoxLayout(generalTab);

    auto* pathGroup = new QGroupBox("Path Settings", generalTab);
    auto* pathLayout = new QFormLayout(pathGroup);

    auto createBrowseRow = [this](const QString& label, QLineEdit*& lineEdit, bool isDir) {
        lineEdit = new QLineEdit();
        auto* btn = new QPushButton("...");
        btn->setFixedWidth(30);
        connect(btn, &QPushButton::clicked, [this, lineEdit, isDir]() {
            QString path = isDir ? QFileDialog::getExistingDirectory(this, "Select Directory")
                                 : QFileDialog::getOpenFileName(this, "Select File");
            if (!path.isEmpty()) lineEdit->setText(path);
        });
        auto* rowLayout = new QHBoxLayout();
        rowLayout->addWidget(lineEdit);
        rowLayout->addWidget(btn);
        return rowLayout;
    };

    pathLayout->addRow("Cache Dir:", createBrowseRow("Cache", m_editCacheDir, true));
    pathLayout->addRow("Log Dir:", createBrowseRow("Log", m_editLogDir, true));

    auto* ttsGroup = new QGroupBox("Text-to-Speech", generalTab);
    auto* ttsLayout = new QFormLayout(ttsGroup);
    m_comboTTSEngine = new QComboBox();
    m_comboTTSEngine->addItems({"gcp", "piper"});
    m_comboLanguage = new QComboBox();
    for (const auto& entry : kTtsLanguages) {
        m_comboLanguage->addItem(entry);
    }
    m_comboVoice = new QComboBox();
    m_comboVoice->setEditable(true);
    m_comboEncoding = new QComboBox();
    m_comboEncoding->addItems({"MP3", "OGG_OPUS", "WAV", "MULAW"});
    m_editPiperModelPath = new QLineEdit();

    ttsLayout->addRow("Engine:", m_comboTTSEngine);

    m_gcpCredRow = new QWidget();
    auto* gcpCredLayout = new QHBoxLayout(m_gcpCredRow);
    gcpCredLayout->setContentsMargins(0, 0, 0, 0);
    gcpCredLayout->addLayout(createBrowseRow("GCPCred", m_editGoogleCredentialPath, false));
    ttsLayout->addRow("Credential:", m_gcpCredRow);

    m_gcpLangRow = new QWidget();
    auto* gcpLangLayout = new QHBoxLayout(m_gcpLangRow);
    gcpLangLayout->setContentsMargins(0, 0, 0, 0);
    gcpLangLayout->addWidget(m_comboLanguage);
    ttsLayout->addRow("Language:", m_gcpLangRow);

    m_gcpVoiceRow = new QWidget();
    auto* gcpVoiceLayout = new QHBoxLayout(m_gcpVoiceRow);
    gcpVoiceLayout->setContentsMargins(0, 0, 0, 0);
    gcpVoiceLayout->addWidget(m_comboVoice);
    ttsLayout->addRow("Voice Name:", m_gcpVoiceRow);

    m_gcpEncRow = new QWidget();
    auto* gcpEncLayout = new QHBoxLayout(m_gcpEncRow);
    gcpEncLayout->setContentsMargins(0, 0, 0, 0);
    gcpEncLayout->addWidget(m_comboEncoding);
    ttsLayout->addRow("Encoding:", m_gcpEncRow);

    m_piperRow = new QWidget();
    auto* piperLayout = new QHBoxLayout(m_piperRow);
    piperLayout->setContentsMargins(0, 0, 0, 0);
    piperLayout->addLayout(createBrowseRow("PiperModel", m_editPiperModelPath, false));
    ttsLayout->addRow("Piper Model:", m_piperRow);

    connect(m_comboTTSEngine, &QComboBox::currentTextChanged,
            [this](const QString&) { updateTTSEngineVisibility(); });
    connect(m_comboLanguage, &QComboBox::currentTextChanged, [this](const QString& display) {
        std::string langCode = languageCodeFromDisplay(display);
        loadVoicesForLanguage(langCode);
    });

    auto* asrGroup = new QGroupBox("Speech Recognition", generalTab);
    auto* asrLayout = new QFormLayout(asrGroup);
    m_editWhisperModelPath = new QLineEdit();
    asrLayout->addRow("Whisper Model:",
                      createBrowseRow("WhisperModel", m_editWhisperModelPath, false));
    m_comboWhisperLanguage = new QComboBox();
    m_comboWhisperLanguage->addItems({"en", "sv", "zh", "ja", "ko"});
    asrLayout->addRow("Language:", m_comboWhisperLanguage);

    auto* configFileGroup = new QGroupBox("Configuration File", generalTab);
    auto* configFileLayout = new QFormLayout(configFileGroup);
    auto* lblConfigPath =
        new QLabel(QString::fromStdString(ConfigStore::instance().getConfigFilePath().string()));
    lblConfigPath->setWordWrap(true);
    lblConfigPath->setTextInteractionFlags(Qt::TextSelectableByMouse);
    configFileLayout->addRow("Location:", lblConfigPath);

    generalLayout->addWidget(pathGroup);
    generalLayout->addWidget(ttsGroup);
    generalLayout->addWidget(asrGroup);
    generalLayout->addWidget(configFileGroup);
    generalLayout->addStretch();

    // --- Tab 2: Network ---
    QWidget* networkTab = new QWidget();
    auto* networkLayout = new QFormLayout(networkTab);
    m_editProxy = new QLineEdit();
    m_editProxy->setPlaceholderText("http://127.0.0.1:7890");
    networkLayout->addRow("HTTP/gRPC Proxy:", m_editProxy);

    // --- Tab 3: About ---
    QWidget* aboutTab = new QWidget();
    auto* aboutLayout = new QVBoxLayout(aboutTab);
    auto* lblAbout = new QLabel(
        "<h2>Thoth Player</h2>"
        "<p>Version: 1.0.0</p>"
        "<p>An industrial-grade TTS player powered by Qt and Google Cloud.</p>"
        "<p>Copyright 2026</p>");
    lblAbout->setAlignment(Qt::AlignCenter);
    aboutLayout->addWidget(lblAbout);

    // Add Tabs
    m_tabWidget->addTab(generalTab, "General");
    m_tabWidget->addTab(networkTab, "Network");
    m_tabWidget->addTab(aboutTab, "About");

    // Buttons
    auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(btnBox, &QDialogButtonBox::accepted, [this]() {
        saveSettings();
        accept();
    });
    connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainLayout->addWidget(m_tabWidget);
    mainLayout->addWidget(btnBox);
}

void Q_SettingDialog::loadSettings() {
    auto& store = ConfigStore::instance();

    auto getStr = [&](const char* key, std::string defValue = "") {
        auto val = store.getValue<std::string>(key);
        return val ? QString::fromStdString(*val) : QString::fromStdString(defValue);
    };

    m_editCacheDir->setText(
        getStr(thoth::config::KEY_CACHE_DIR, store.getCacheDir().string().c_str()));
    m_editLogDir->setText(getStr(thoth::config::KEY_LOG_DIR, store.getLogDir().string().c_str()));
    m_editGoogleCredentialPath->setText(getStr(thoth::config::KEY_GOOGLE_CREDENTIAL_PATH));

    std::string savedLang =
        getStr(thoth::config::KEY_TTS_LANG, thoth::config::DEFAULT_TTS_LANG).toStdString();
    int langIdx = m_comboLanguage->findText(QString::fromStdString(savedLang), Qt::MatchContains);
    if (langIdx >= 0) m_comboLanguage->setCurrentIndex(langIdx);

    m_comboVoice->setCurrentText(
        getStr(thoth::config::KEY_TTS_VOICE, thoth::config::DEFAULT_TTS_VOICE));
    m_comboEncoding->setCurrentText(
        getStr(thoth::config::KEY_TTS_AUDIO_ENCODING, thoth::config::DEFAULT_TTS_AUDIO_ENCODING));
    m_comboTTSEngine->setCurrentText(
        getStr(thoth::config::KEY_TTS_ENGINE, thoth::config::DEFAULT_TTS_ENGINE));
    m_editPiperModelPath->setText(getStr(thoth::config::KEY_TTS_PIPER_MODEL_PATH,
                                         thoth::config::DEFAULT_TTS_PIPER_MODEL_PATH));
    m_editWhisperModelPath->setText(
        getStr(thoth::config::KEY_WHISPER_MODEL_PATH, thoth::config::DEFAULT_WHISPER_MODEL_PATH));
    m_comboWhisperLanguage->setCurrentText(getStr(thoth::config::KEY_WHISPER_MODEL_LANGUAGE,
                                                  thoth::config::DEFAULT_WHISPER_MODEL_LANGUAGE));

    m_editProxy->setText(getStr(thoth::config::KEY_PROXY));
    updateTTSEngineVisibility();

    std::string initialLang = languageCodeFromDisplay(m_comboLanguage->currentText());
    loadVoicesForLanguage(initialLang);
}

void Q_SettingDialog::saveSettings() {
    auto& store = ConfigStore::instance();
    bool needsRestart = false;

    auto oldCache = store.getValue<std::string>(thoth::config::KEY_CACHE_DIR);
    auto oldLog = store.getValue<std::string>(thoth::config::KEY_LOG_DIR);
    auto oldSampleRate = store.getValue<uint32_t>(thoth::config::KEY_AUDIO_RECORDER_SAMPLE_RATE);

    store.setValue(thoth::config::KEY_CACHE_DIR, m_editCacheDir->text().toStdString());
    store.setValue(thoth::config::KEY_LOG_DIR, m_editLogDir->text().toStdString());
    store.setValue(thoth::config::KEY_GOOGLE_CREDENTIAL_PATH,
                   m_editGoogleCredentialPath->text().toStdString());

    store.setValue(thoth::config::KEY_TTS_LANG,
                   languageCodeFromDisplay(m_comboLanguage->currentText()));
    store.setValue(thoth::config::KEY_TTS_VOICE, m_comboVoice->currentText().toStdString());
    store.setValue(thoth::config::KEY_TTS_AUDIO_ENCODING,
                   m_comboEncoding->currentText().toStdString());
    store.setValue(thoth::config::KEY_TTS_ENGINE, m_comboTTSEngine->currentText().toStdString());
    store.setValue(thoth::config::KEY_TTS_PIPER_MODEL_PATH,
                   m_editPiperModelPath->text().toStdString());
    store.setValue(thoth::config::KEY_WHISPER_MODEL_PATH,
                   m_editWhisperModelPath->text().toStdString());
    store.setValue(thoth::config::KEY_WHISPER_MODEL_LANGUAGE,
                   m_comboWhisperLanguage->currentText().toStdString());

    store.setValue(thoth::config::KEY_PROXY, m_editProxy->text().toStdString());

    if (!m_editProxy->text().isEmpty()) {
        QByteArray proxyBytes = m_editProxy->text().toLocal8Bit();
        qputenv("http_proxy", proxyBytes);
        qputenv("https_proxy", proxyBytes);
        qputenv("grpc_proxy", proxyBytes);
    }

    LOG_DEBUG("Settings updated, current settings: {}", store);
}

void Q_SettingDialog::updateTTSEngineVisibility() {
    bool isGcp = m_comboTTSEngine->currentText() == "gcp";
    m_gcpCredRow->setVisible(isGcp);
    m_gcpLangRow->setVisible(isGcp);
    m_gcpVoiceRow->setVisible(isGcp);
    m_gcpEncRow->setVisible(isGcp);
    m_piperRow->setVisible(!isGcp);
}

void Q_SettingDialog::loadVoicesForLanguage(const std::string& languageCode) {
    if (!m_voiceLoader) {
        m_voiceLoader = std::make_unique<Q_GCPVoiceLoader>(this);
        connect(
            m_voiceLoader.get(), &Q_GCPVoiceLoader::voicesReady, this,
            [this](const std::string& langCode, const std::vector<thoth::GoogleVoiceInfo>& voices) {
                if (langCode != languageCodeFromDisplay(m_comboLanguage->currentText())) return;

                if (voices.empty()) return;

                m_loadedVoices = voices;
                m_loadedLanguage = langCode;
                QString savedVoice = m_comboVoice->currentText();
                m_comboVoice->clear();
                bool savedFound = false;
                for (const auto& v : voices) {
                    QString name = QString::fromStdString(v.name);
                    m_comboVoice->addItem(name);
                    if (name == savedVoice) savedFound = true;
                }
                if (savedFound) {
                    m_comboVoice->setCurrentText(savedVoice);
                } else {
                    m_comboVoice->setCurrentIndex(0);
                }
            });
        connect(m_voiceLoader.get(), &Q_GCPVoiceLoader::loadError, this,
                [this](const std::string& langCode, const QString& errorMsg) {
                    if (langCode != languageCodeFromDisplay(m_comboLanguage->currentText())) return;
                    LOG_WARN("Voice loading failed for language '{}': {}", langCode,
                             errorMsg.toStdString());
                });
    }

    auto& config = ConfigStore::instance();
    m_voiceLoader->loadVoices(languageCode, config.getGoogleTTSConfig());
}
