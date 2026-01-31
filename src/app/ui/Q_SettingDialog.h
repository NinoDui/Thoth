#pragma once

#include <QComboBox>
#include <QDialog>
#include <QLineEdit>
#include <QTabWidget>

class Q_SettingDialog : public QDialog {
    Q_OBJECT
   public:
    explicit Q_SettingDialog(QWidget* parent = nullptr);
    void saveSettings();

   private:
    void setupUI();
    void loadSettings();

    // UI Components
    QTabWidget* m_tabWidget;

    // General Tab
    QLineEdit* m_editCacheDir;
    QLineEdit* m_editLogDir;
    QLineEdit* m_editGoogleCredentialPath;
    QComboBox* m_comboLanguage;
    QComboBox* m_comboVoice;

    // Network Tab
    QLineEdit* m_editProxy;
};