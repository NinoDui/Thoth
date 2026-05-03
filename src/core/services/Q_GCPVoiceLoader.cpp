#include "thoth/Q_GCPVoiceLoader.h"

#include <QtConcurrent>

#include "internal/gcp.h"
#include "thoth/Logger.h"

void Q_GCPVoiceLoader::loadVoices(const std::string& languageCode,
                                  const thoth::GoogleTTSConfig& config) {
    uint64_t requestId = ++m_requestId;
    m_pendingLanguage = languageCode;

    auto* watcher = new QFutureWatcher<std::vector<thoth::GoogleVoiceInfo>>(this);
    connect(watcher, &QFutureWatcher<std::vector<thoth::GoogleVoiceInfo>>::finished, this,
            [this, watcher, languageCode, requestId]() {
                watcher->deleteLater();

                if (requestId != m_requestId) {
                    return;
                }

                try {
                    auto results = watcher->future().result();
                    emit voicesReady(languageCode, results);
                } catch (const std::exception& e) {
                    LOG_ERROR("Voice loading failed for '{}': {}", languageCode, e.what());
                    emit loadError(languageCode, QString::fromStdString(e.what()));
                } catch (...) {
                    LOG_ERROR("Voice loading failed for '{}': unknown error", languageCode);
                    emit loadError(languageCode, "Unknown error loading voices");
                }
            });

    QFuture<std::vector<thoth::GoogleVoiceInfo>> future =
        QtConcurrent::run([config, languageCode]() -> std::vector<thoth::GoogleVoiceInfo> {
            GCPTextToSpeechClient client(config);
            return client.listVoices(languageCode);
        });

    watcher->setFuture(future);
    m_watcher = watcher;
}
