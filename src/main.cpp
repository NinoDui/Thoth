#include <math.h>

#include <QCoreApplication>
#include <QTimer>
#include <iostream>

#include "core/Logger.h"
#include "core/Q_GCPTTSDownloader.h"
#include "core/TextParser.h"

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    LogManager::Guard logGuard("ThothApp");

    std::string test_text = "Hello, world!";

    Q_GCPTTSDownloader downloader;
    try {
        bool download_completed = false;
        downloader.download(test_text, [&](bool success, QByteArray data) {
            if (success) {
                LOG_INFO("Downloaded {} bytes", data.size());
            } else {
                LOG_ERROR("Failed to download");
            }
            download_completed = true;
            app.quit();
        });
    } catch (const std::exception& e) {
        LOG_ERROR("Error: {}", e.what());
        return 1;
    }

    return app.exec();
}