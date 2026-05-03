#include "AppRunner.h"

#include <QFileDialog>
#include <QMessageBox>

#include "thoth/AuthProviderFactory.h"
#include "thoth/ConfigKey.h"
#include "thoth/ConfigStore.h"
#include "thoth/Logger.h"
#include "ui/Q_AppMainWindow.h"
#include "ui/Q_SettingDialog.h"
#include "ui/StyleLoader.h"

AppRunner::AppRunner(int& argc, char** argv) : m_argc(argc), m_argv(argv) {
    // ConfigStore initialized, but it using the default log settings
    ConfigStore::instance().init();
    // we have to ensure log is initialized FIRST!!!
    // log settings is reset by config under ConfigStore
    m_logGuard = std::make_unique<LogManager::Guard>("ThothApp");
}

AppRunner::~AppRunner() = default;

void AppRunner::initEnv() {
    LOG_INFO("Initializing environment...");
    LOG_INFO("Configuration details: {}", ConfigStore::instance());

    // 2. Initialize application
    m_app = std::make_unique<QApplication>(m_argc, m_argv);
    m_app->setApplicationName("Thoth");
    StyleLoader::attach();
}

bool AppRunner::setupNetwork() {
    auto proxy = ConfigStore::instance().getValue<std::string>(thoth::config::KEY_PROXY);
    if (proxy && !proxy->empty()) {
        QByteArray p = proxy->c_str();
        qputenv("http_proxy", p);
        qputenv("https_proxy", p);
        qputenv("grpc_proxy", p);
        LOG_INFO("Proxy set to: {}", *proxy);
        return true;
    } else {
        LOG_WARN("No proxy found, the Google API is not workable.");
        return false;
    }
}

bool AppRunner::authenticate() {
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
    return authChain->execute();
}

int AppRunner::run() {
    try {
        initEnv();
        setupNetwork();

        if (!authenticate()) {
            LOG_CRITICAL("Authentication failed, exiting...");
            QMessageBox::critical(nullptr, "Authentication Failed",
                                  "Authentication failed, exiting...");
            return 1;
        }

        LOG_INFO("Starting the main window ... ");
        Q_AppMainWindow mainWindow;
        mainWindow.show();

        return m_app->exec();
    } catch (const std::exception& e) {
        LOG_CRITICAL("Unhandled Exception: {}", e.what());
        QMessageBox::critical(nullptr, "Critical Error", e.what());
        return 1;
    } catch (...) {
        LOG_CRITICAL("Unknown crash occurred, exiting ...");
        QMessageBox::critical(nullptr, "Crash", "Unknown crash occurred, exiting ...");
        return 1;
    }
}
