#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>

#include "Q_AppMainWindow.h"
#include "thoth/AuthProviderFactory.h"
#include "thoth/ConfigStore.h"
#include "thoth/Logger.h"

int main(int argc, char* argv[]) {
    // 1. Initialize logger
    LogManager::Guard logGuard("ThothApp");
    LOG_INFO("Starting ThothApp...");

    // 2. Initialize configuration
    ConfigStore::instance().init();
    LOG_INFO("Configuration initialized, details: {}", ConfigStore::instance());

    QApplication app(argc, argv);

    AuthCallbacks callbacks;
    callbacks.onRequestFile = []() -> std::filesystem::path {
        QMessageBox::information(nullptr, "Setup",
                                 "Please select the Google Credential File for your account.");
        QString fileName = QFileDialog::getOpenFileName(nullptr, "Select Credential", "",
                                                        "Credential Files (*.json)");
        if (fileName.isEmpty()) return {};
        return std::filesystem::path(fileName.toStdString());
    };
    // not set login callback for now, skip this step
    auto authChain = AuthProviderFactory::createAuthChain(callbacks);
    if (!authChain->execute()) {
        LOG_CRITICAL("Authentication failed, exiting...");
        QMessageBox::critical(nullptr, "Authentication Failed",
                              "Authentication failed, exiting...");
        app.quit();
        return 1;
    }
    LOG_INFO("Authentication successful, starting application...");

    try {
        Q_AppMainWindow mainWindow;
        mainWindow.show();
        return app.exec();
    } catch (const std::exception& e) {
        LOG_CRITICAL("Unhandled Exception: {}", e.what());
        QMessageBox::critical(nullptr, "Critical Error",
                              QString("Unhandled Exception:\n%1").arg(e.what()));
        return 1;
    } catch (...) {
        LOG_CRITICAL("Unknown crash occurred, exiting ...");
        QMessageBox::critical(nullptr, "Crash", "Unknown crash occurred, exiting ...");
        return 1;
    }
}