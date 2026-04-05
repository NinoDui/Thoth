#include "thoth/Q_ASRController.h"

#include "internal/Q_WhisperWorker.h"
#include "thoth/ISentenceScorer.h"
#include "thoth/Logger.h"

Q_ASRController::Q_ASRController(ISentenceScorer* scorer, QObject* parent)
    : QObject(parent), m_scorer(scorer) {
    m_worker = new Q_WhisperWorker();
    m_workerThread = new QThread(this);
    m_worker->moveToThread(m_workerThread);

    connect(m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(this, &Q_ASRController::sigTranscribe, m_worker, &Q_WhisperWorker::doTranscribe);
    connect(m_worker, &Q_WhisperWorker::transcriptReady, this,
            &Q_ASRController::onTranscriptReady);
    connect(m_worker, &Q_WhisperWorker::busyChanged, this, &Q_ASRController::busyChanged);
    connect(m_worker, &Q_WhisperWorker::errorOccurred, this, &Q_ASRController::errorOccurred);

    m_workerThread->start();
}

Q_ASRController::~Q_ASRController() {
    m_workerThread->quit();
    m_workerThread->wait();
}

void Q_ASRController::analyze(RecordedSentence* rs) {
    emit sigTranscribe(rs);
}

void Q_ASRController::onTranscriptReady(RecordedSentence* rs) {
    if (rs->transcribedText && !rs->transcribedText->empty()) {
        rs->shadowingScore = m_scorer->score(rs->text, *rs->transcribedText);
        LOG_INFO("Score for sentence [{}]: {:.2f}%", rs->id, rs->shadowingScore * 100.0);
    } else {
        rs->shadowingScore = 0.0;
        LOG_WARN("Empty transcription for sentence [{}], score set to 0", rs->id);
    }
    emit analysisReady(rs);
}
