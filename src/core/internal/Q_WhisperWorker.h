#pragma once

#include <QObject>
#include <QString>

#include "thoth/Entity.h"
#include "thoth/ThothConfig.h"

struct whisper_context;

class Q_WhisperWorker : public QObject {
    Q_OBJECT
   public:
    explicit Q_WhisperWorker(const thoth::WhisperConfig& config, QObject* parent = nullptr);
    ~Q_WhisperWorker();

   public slots:
    void doTranscribe(RecordedSentence* rs);
    void doTranscribeFile(const QString& audioPath);
    void reloadModel(const thoth::WhisperConfig& config);

   signals:
    void transcriptReady(RecordedSentence* rs);
    void transcriptSegmentsReady(std::vector<TranscriptSegment> segments);
    void busyChanged(bool busy);
    void errorOccurred(const QString& message);

   private:
    bool ensureContext();

    whisper_context* m_ctx = nullptr;
    thoth::WhisperConfig m_config;
};
