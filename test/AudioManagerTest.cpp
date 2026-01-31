#include <QCoreApplication>
#include <QObject>
#include <QTimer>

#include "thoth/Logger.h"
#include "thoth/Q_AudioManager.h"

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    LogManager::Guard logGuard("ThothAudioManagerTest");

    std::vector<std::string> test_texts = {"Hello, world!", "This is Ashin from Mayday.",
                                           "Keep your dream, ever youth, ever weeping."};

    Q_AudioManager audioManager;
    LOG_DEBUG("Audio Manager is created, expected cache dir: {}",
              audioManager.getCacheDir().string());

    try {
        QObject::connect(&audioManager, &Q_AudioManager::playbackStarted, [&](int idx) {
            std::string text = test_texts[idx];
            LOG_DEBUG(">> Playback STARTED for index [{}]: \"{}\"", idx, text);
        });

        QObject::connect(&audioManager, &Q_AudioManager::playbackFinished, [&](int idx) {
            LOG_DEBUG(">> Playback FINISHED for index [{}]: \"{}\"", idx, test_texts[idx]);

            if (idx == test_texts.size() - 1) {
                LOG_DEBUG(">> Playback FINISHED for all sentences");
                app.quit();
            }
        });

        audioManager.setPlaylist(test_texts);
        LOG_INFO(">> Playlist is set, total {} sentences", test_texts.size());

        audioManager.play(0);

    } catch (const std::exception& e) {
        LOG_ERROR("Error: {}", e.what());
        return 1;
    }

    return app.exec();
}