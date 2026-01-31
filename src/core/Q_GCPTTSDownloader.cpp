#include "internal/Q_GCPTTSDownloader.h"

#include <QFutureWatcher>
#include <QtConcurrent>
#include <iostream>

#include "thoth/Logger.h"

Q_GCPTTSDownloader::Q_GCPTTSDownloader(QObject* parent)
    : QObject(parent), m_ttsClient(std::make_shared<GCPTextToSpeechClient>()) {}

Q_GCPTTSDownloader::Q_GCPTTSDownloader(const std::shared_ptr<GCPRuntimeConfig>& config,
                                       QObject* parent)
    : QObject(parent), m_ttsClient(std::make_shared<GCPTextToSpeechClient>(config)) {}

void Q_GCPTTSDownloader::download(const std::string& text, const DownloadCallback& callback) {
    // explicitly capture the synchronous client
    // keep it alive even the external asynchnornous calling thread is finished
    auto clientPtr = m_ttsClient;

    // QtObject, safe op to use 'new'
    auto* watcher = new QFutureWatcher<QByteArray>(this);
    connect(watcher, &QFutureWatcher<QByteArray>::finished, this, [watcher, callback]() {
        QByteArray result = watcher->result();
        bool success = !result.isEmpty();

        if (callback) {
            callback(success, result);
        }

        watcher->deleteLater();
    });

    QFuture<QByteArray> future = QtConcurrent::run([clientPtr, text]() -> QByteArray {
        try {
            std::vector<uint8_t> audioData = clientPtr->execute(text);
            QByteArray bytes(reinterpret_cast<const char*>(audioData.data()), audioData.size());
            return bytes;
        } catch (const std::exception& e) {
            LOG_ERROR("GCP Text-to-Speech Error: {}", e.what());
            return QByteArray();
        }
    });
    watcher->setFuture(future);
}