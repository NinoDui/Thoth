#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <filesystem>
#include <functional>
#include <qtmetamacros.h>

class TTSDownloader : public QObject {
    Q_OBJECT
public:
    explicit TTSDownloader(QObject* parent = nullptr);

    using DownloadCallback = std::function<void(bool success, std::filesystem::path path)>;

    void download(const std::string& text, const std::filesystem::path& dst, DownloadCallback callback);

private:
    QNetworkAccessManager* m_manager;
};