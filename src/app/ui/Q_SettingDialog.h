#pragma once

#include <QComboBox>
#include <QDialog>
#include <QLineEdit>
#include <QTabWidget>
#include <QWidget>
#include <memory>
#include <string>
#include <vector>

#include "thoth/Q_GCPVoiceLoader.h"
#include "thoth/ThothConfig.h"

class Q_SettingDialog : public QDialog {
    Q_OBJECT
   public:
    explicit Q_SettingDialog(QWidget* parent = nullptr);
    ~Q_SettingDialog() override;
    void saveSettings();

   private:
    void setupUI();
    void loadSettings();
    void loadVoicesForLanguage(const std::string& languageCode);
    void updateTTSEngineVisibility();

    // UI Components
    QTabWidget* m_tabWidget;

    // General Tab
    QLineEdit* m_editCacheDir;
    QLineEdit* m_editLogDir;
    QLineEdit* m_editGoogleCredentialPath;
    QComboBox* m_comboLanguage;
    QComboBox* m_comboVoice;
    QComboBox* m_comboTTSEngine;
    QComboBox* m_comboEncoding;
    QLineEdit* m_editPiperModelPath;
    QLineEdit* m_editWhisperModelPath;
    QComboBox* m_comboWhisperLanguage;

    QWidget* m_gcpCredRow = nullptr;
    QWidget* m_gcpLangRow = nullptr;
    QWidget* m_gcpVoiceRow = nullptr;
    QWidget* m_gcpEncRow = nullptr;
    QWidget* m_piperRow = nullptr;

    // Network Tab
    QLineEdit* m_editProxy;

    // Voice discovery
    std::unique_ptr<Q_GCPVoiceLoader> m_voiceLoader;
    std::vector<thoth::GoogleVoiceInfo> m_loadedVoices;
    std::string m_loadedLanguage;
};
