#include "internal/Q_GCPTTSDownloader.h"

#include <QFutureWatcher>
#include <QPair>
#include <QtConcurrent>

#include "thoth/Logger.h"

using DownloadResult = QPair<QByteArray, QString>;

Q_TTSDownloader::Q_TTSDownloader(std::shared_ptr<thoth::ITTSEngine> engine, QObject* parent)
    : QObject(parent), m_engine(std::move(engine)) {}

void Q_TTSDownloader::download(const std::string& text, const DownloadCallback& callback) {
    auto enginePtr = m_engine;

    auto* watcher = new QFutureWatcher<DownloadResult>(this);
    connect(watcher, &QFutureWatcher<DownloadResult>::finished, this, [watcher, callback]() {
        DownloadResult result = watcher->result();
        bool success = !result.first.isEmpty();

        if (callback) {
            callback(success, result.first, result.second);
        }

        watcher->deleteLater();
    });

    QFuture<DownloadResult> future = QtConcurrent::run([enginePtr, text]() -> DownloadResult {
        try {
            auto result = enginePtr->synthesize(text);
            QByteArray bytes(reinterpret_cast<const char*>(result.audioData.data()),
                             result.audioData.size());
            return {bytes, QString()};
        } catch (const std::exception& e) {
            LOG_ERROR("TTS synthesis error: {}", e.what());
            return {QByteArray(), QString::fromStdString(e.what())};
        }
    });
    watcher->setFuture(future);
}
