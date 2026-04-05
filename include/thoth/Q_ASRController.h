#pragma once

#include <QObject>
#include <QThread>

#include "thoth/Entity.h"

class ISentenceScorer;
class Q_WhisperWorker;

class Q_ASRController : public QObject {
    Q_OBJECT
   public:
    explicit Q_ASRController(ISentenceScorer* scorer, QObject* parent = nullptr);
    ~Q_ASRController();

    void analyze(RecordedSentence* rs);

   signals:
    void analysisReady(RecordedSentence* rs);
    void busyChanged(bool busy);
    void errorOccurred(const QString& message);

    // internal — bridges main thread to worker thread
    void sigTranscribe(RecordedSentence* rs);

   private slots:
    void onTranscriptReady(RecordedSentence* rs);

   private:
    ISentenceScorer* m_scorer;
    Q_WhisperWorker* m_worker;
    QThread* m_workerThread;
};
