#pragma once

#include <QFutureWatcher>
#include <QObject>
#include <QPointer>
#include <cstdint>
#include <string>
#include <vector>

#include "thoth/ThothConfig.h"

class Q_GCPVoiceLoader : public QObject {
    Q_OBJECT
   public:
    explicit Q_GCPVoiceLoader(QObject* parent = nullptr) : QObject(parent) {}

    void loadVoices(const std::string& languageCode, const thoth::GoogleTTSConfig& config);

   signals:
    void voicesReady(const std::string& languageCode,
                     const std::vector<thoth::GoogleVoiceInfo>& voices);
    void loadError(const std::string& languageCode, const QString& errorMsg);

   private:
    QPointer<QFutureWatcher<std::vector<thoth::GoogleVoiceInfo>>> m_watcher;
    uint64_t m_requestId = 0;
    std::string m_pendingLanguage;
};
