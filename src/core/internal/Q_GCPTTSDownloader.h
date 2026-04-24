#pragma once

#include <QByteArray>
#include <QObject>
#include <functional>
#include <memory>
#include <string>

#include "thoth/ITTSEngine.h"

class Q_TTSDownloader : public QObject {
    Q_OBJECT
   public:
    explicit Q_TTSDownloader(std::shared_ptr<thoth::ITTSEngine> engine, QObject* parent = nullptr);
    ~Q_TTSDownloader() = default;

    using DownloadCallback = std::function<void(bool success, QByteArray data, QString errorMsg)>;
    void download(const std::string& text, const DownloadCallback& callback);

   private:
    std::shared_ptr<thoth::ITTSEngine> m_engine;
};

using Q_GCPTTSDownloader = Q_TTSDownloader;
