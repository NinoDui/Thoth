#pragma once

#include <QByteArray>
#include <QObject>
#include <functional>
#include <memory>
#include <string>

#include "internal/GCP.h"

/**
 * @brief The Q_GCPTTSDownloader class is a Qt wrapper for the GCPTextToSpeechClient class.
 * It is used to download text to speech audio from the GCP Text to Speech API.
 * This class wraps the synchronous GCPTextToSpeechClient class into a asynchronous API,
 * which is compatible with AudioManager class, a Qt-oriented class.
 */
class Q_GCPTTSDownloader : public QObject {
    Q_OBJECT
   public:
    explicit Q_GCPTTSDownloader(QObject* parent = nullptr);
    ~Q_GCPTTSDownloader() = default;

    using DownloadCallback = std::function<void(bool success, QByteArray data)>;
    void download(const std::string& text, const DownloadCallback& callback);

   private:
    // shared_ptr to be compatible with Qt's asynchronous API
    // keep the object alive until the callback is returned
    std::shared_ptr<GCPTextToSpeechClient> m_ttsClient;
};