#include "internal/Q_GCPTTSDownloader.h"

#include <QFutureWatcher>
#include <QPair>
#include <QtConcurrent>

#include "thoth/Logger.h"

using DownloadResult = QPair<QByteArray, QString>;

Q_GCPTTSDownloader::Q_GCPTTSDownloader(QObject* parent)
    : QObject(parent), m_ttsClient(std::make_shared<GCPTextToSpeechClient>()) {}

void Q_GCPTTSDownloader::download(const std::string& text, const DownloadCallback& callback) {
    // explicitly capture the synchronous client
    // keep it alive even the external asynchnornous calling thread is finished
    auto clientPtr = m_ttsClient;

    // QtObject, safe op to use 'new'
    auto* watcher = new QFutureWatcher<DownloadResult>(this);
    connect(watcher, &QFutureWatcher<DownloadResult>::finished, this, [watcher, callback]() {
        DownloadResult result = watcher->result();
        bool success = !result.first.isEmpty();

        if (callback) {
            callback(success, result.first, result.second);
        }

        watcher->deleteLater();
    });

    QFuture<DownloadResult> future = QtConcurrent::run([clientPtr, text]() -> DownloadResult {
        try {
            std::vector<uint8_t> audioData = clientPtr->execute(text);
            QByteArray bytes(reinterpret_cast<const char*>(audioData.data()), audioData.size());
            return {bytes, QString()};
        } catch (const std::exception& e) {
            LOG_ERROR("GCP Text-to-Speech Error: {}", e.what());
            return {QByteArray(), QString::fromStdString(e.what())};
        }
    });
    watcher->setFuture(future);
}