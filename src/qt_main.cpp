#include <QCoreApplication>
#include <QTimer>
#include <iostream>

#include "core/TextParser.h"
#include "thoth/Q_TTSDownloader.h"

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    try {
        std::string test_text = "Hello, world!";

        TTSDownloader downloader;
        bool download_completed = false;

        std::cout << "Downloading..." << std::endl;
        std::cout << "Test text: " << test_text << std::endl;

        downloader.download(test_text, [&](bool success, QByteArray data) {
            if (success) {
                std::cout << "Downloaded " << data.size() << " bytes" << std::endl;
            } else {
                std::cerr << "Failed to download" << std::endl;
            }
            download_completed = true;
            app.quit();
        });

        QTimer::singleShot(30000, &app, [&]() {
            if (!download_completed) {
                std::cerr << "Timeout, download not completed" << std::endl;
                app.quit();
            }
        });
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return app.exec();
}