#pragma once
#include <qtmetamacros.h>

#include <QNetworkAccessManager>
#include <QObject>
#include <filesystem>
#include <functional>

class TTSDownloader : public QObject {
    Q_OBJECT
   public:
    explicit TTSDownloader(QNetworkAccessManager* qNetworkManager, QObject* parent = nullptr);
    explicit TTSDownloader(QObject* parent = nullptr);

    using DownloadCallback = std::function<void(bool success, QByteArray data)>;
    void download(const std::string& text, DownloadCallback callback);

   private:
    QNetworkAccessManager* m_qNetworkManager;
};