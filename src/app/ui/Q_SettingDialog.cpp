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

Q_SettingDialog::Q_SettingDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Settings");
    resize(400, 300);
    setupUI();
    loadSettings();
}

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
    m_comboLanguage->addItems({"en-US", "zh-CN", "ja-JP"});
    m_comboVoice = new QComboBox();
    m_comboVoice->setEditable(true);
    m_editPiperModelPath = new QLineEdit();

    ttsLayout->addRow("Engine:", m_comboTTSEngine);
    ttsLayout->addRow("Credential:", createBrowseRow("GCPCred", m_editGoogleCredentialPath, false));
    ttsLayout->addRow("Language:", m_comboLanguage);
    ttsLayout->addRow("Voice Name:", m_comboVoice);
    ttsLayout->addRow("Piper Model:", createBrowseRow("PiperModel", m_editPiperModelPath, false));

    auto* configFileGroup = new QGroupBox("Configuration File", generalTab);
    auto* configFileLayout = new QFormLayout(configFileGroup);
    auto* lblConfigPath =
        new QLabel(QString::fromStdString(ConfigStore::instance().getConfigFilePath().string()));
    lblConfigPath->setWordWrap(true);
    lblConfigPath->setTextInteractionFlags(Qt::TextSelectableByMouse);
    configFileLayout->addRow("Location:", lblConfigPath);

    generalLayout->addWidget(pathGroup);
    generalLayout->addWidget(ttsGroup);
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

    m_comboLanguage->setCurrentText(
        getStr(thoth::config::KEY_TTS_LANG, thoth::config::DEFAULT_TTS_LANG));
    m_comboVoice->setCurrentText(
        getStr(thoth::config::KEY_TTS_VOICE, thoth::config::DEFAULT_TTS_VOICE));
    m_comboTTSEngine->setCurrentText(
        getStr(thoth::config::KEY_TTS_ENGINE, thoth::config::DEFAULT_TTS_ENGINE));
    m_editPiperModelPath->setText(getStr(thoth::config::KEY_TTS_PIPER_MODEL_PATH,
                                         thoth::config::DEFAULT_TTS_PIPER_MODEL_PATH));

    m_editProxy->setText(getStr(thoth::config::KEY_PROXY));
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

    store.setValue(thoth::config::KEY_TTS_LANG, m_comboLanguage->currentText().toStdString());
    store.setValue(thoth::config::KEY_TTS_VOICE, m_comboVoice->currentText().toStdString());
    store.setValue(thoth::config::KEY_TTS_ENGINE, m_comboTTSEngine->currentText().toStdString());
    store.setValue(thoth::config::KEY_TTS_PIPER_MODEL_PATH,
                   m_editPiperModelPath->text().toStdString());

    store.setValue(thoth::config::KEY_PROXY, m_editProxy->text().toStdString());

    if (!m_editProxy->text().isEmpty()) {
        QByteArray proxyBytes = m_editProxy->text().toLocal8Bit();
        qputenv("http_proxy", proxyBytes);
        qputenv("https_proxy", proxyBytes);
        qputenv("grpc_proxy", proxyBytes);
    }

    LOG_DEBUG("Settings updated, current settings: {}", store);
}
