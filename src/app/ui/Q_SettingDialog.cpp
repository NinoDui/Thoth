#include "Q_SettingDialog.h"

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
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

    auto* ttsGroup = new QGroupBox("Google TTS", generalTab);
    auto* ttsLayout = new QFormLayout(ttsGroup);
    m_comboLanguage = new QComboBox();
    m_comboLanguage->addItems({"en-US", "zh-CN", "ja-JP"});
    m_comboVoice = new QComboBox();
    m_comboVoice->setEditable(true);

    ttsLayout->addRow("Credential:", createBrowseRow("GCPCred", m_editGoogleCredentialPath, false));
    ttsLayout->addRow("Language:", m_comboLanguage);
    ttsLayout->addRow("Voice Name:", m_comboVoice);

    generalLayout->addWidget(pathGroup);
    generalLayout->addWidget(ttsGroup);
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

    m_editProxy->setText(getStr(thoth::config::KEY_PROXY));
}

void Q_SettingDialog::saveSettings() {
    auto& store = ConfigStore::instance();

    store.setValue(thoth::config::KEY_CACHE_DIR, m_editCacheDir->text().toStdString());
    store.setValue(thoth::config::KEY_LOG_DIR, m_editLogDir->text().toStdString());
    store.setValue(thoth::config::KEY_GOOGLE_CREDENTIAL_PATH,
                   m_editGoogleCredentialPath->text().toStdString());

    store.setValue(thoth::config::KEY_TTS_LANG, m_comboLanguage->currentText().toStdString());
    store.setValue(thoth::config::KEY_TTS_VOICE, m_comboVoice->currentText().toStdString());

    store.setValue(thoth::config::KEY_PROXY, m_editProxy->text().toStdString());

    if (!m_editProxy->text().isEmpty()) {
        QByteArray proxyBytes = m_editProxy->text().toLocal8Bit();
        qputenv("http_proxy", proxyBytes);
        qputenv("https_proxy", proxyBytes);
        qputenv("grpc_proxy", proxyBytes);
        LOG_INFO("Proxy settings updated dynamically.");
    }

    LOG_DEBUG("Settings updated, current settings: {}", store);
}