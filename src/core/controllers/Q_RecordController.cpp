#include "thoth/Q_RecordController.h"

#include <QDebug>

#include "internal/InternalEntity.h"
#include "internal/LockFreeRingBuffer.h"
#include "internal/Q_AudioCapture.h"
#include "internal/Q_AudioStorage.h"
#include "internal/Timer.h"
#include "thoth/Entity.h"
#include "thoth/Logger.h"

Q_RecordController::Q_RecordController(const QAudioFormat& format, uint16_t rmsStep,
                                       QObject* parent)
    : QObject(parent),
      m_captureProducer(std::make_unique<Q_AudioCaptureProducer>(format)),
      m_rmsStep(rmsStep) {
    m_ringBuffer = std::make_unique<LockFreeRingBuffer>(DEFAULT_RING_BUFFER_SIZE);
    m_streamSaver = new AudioFileStreamSaver(m_ringBuffer.get(), m_captureProducer->format());
    setupConnections();
}

Q_RecordController::Q_RecordController(std::unique_ptr<Q_AudioCaptureProducer> captureProducer,
                                       AudioFileStreamSaver* streamSaver,
                                       std::unique_ptr<LockFreeRingBuffer> ringBuffer,
                                       uint16_t rmsStep, QObject* parent)
    : QObject(parent),
      m_captureProducer(std::move(captureProducer)),
      m_streamSaver(streamSaver),
      m_ringBuffer(std::move(ringBuffer)),
      m_rmsStep(rmsStep) {
    setupConnections();
}

Q_RecordController::~Q_RecordController() {
    stopRecording();
    if (m_ioWorkerThread) {
        m_ioWorkerThread->quit();
        m_ioWorkerThread->wait();
    }
}

void Q_RecordController::setupConnections() {
    // launch the IO thread
    m_ioWorkerThread = new QThread(this);
    m_streamSaver->moveToThread(m_ioWorkerThread);

    // connections with the capture producer -> Controller
    connect(m_captureProducer.get(), &Q_AudioCaptureProducer::audioDataAvailable, this,
            &Q_RecordController::onAudioDataAvailable);
    connect(m_captureProducer.get(), &Q_AudioCaptureProducer::errorOccurred, this,
            &Q_RecordController::errorOccurred);

    // connections Controller -> Storage (cross thread)
    connect(this, &Q_RecordController::sigStartSession, m_streamSaver,
            &AudioFileStreamSaver::startSession);
    connect(this, &Q_RecordController::sigStopSession, m_streamSaver,
            &AudioFileStreamSaver::finalizeSession);
    connect(this, &Q_RecordController::sigAbortSession, m_streamSaver,
            &AudioFileStreamSaver::abortSession);

    // Storage (Consumer) -> Controller
    connect(m_streamSaver, &AudioFileStreamSaver::sessionFinalized, this,
            &Q_RecordController::onSessionFinalized);
    connect(m_streamSaver, &AudioFileStreamSaver::sessionAborted, this,
            &Q_RecordController::onSessionAborted);
    connect(m_streamSaver, &AudioFileStreamSaver::errorOccurred, this,
            &Q_RecordController::onStorageError);

    // IO thread life cycle
    connect(m_ioWorkerThread, &QThread::finished, m_streamSaver, &QObject::deleteLater);
    m_ioWorkerThread->start();
}

bool Q_RecordController::startRecording(const std::string& sessionId) {
    if (m_isRecording) return false;

    if (!m_captureProducer->start()) {
        LOG_ERROR("Failed to start audio capture");
        emit errorOccurred("Failed to start audio capture");
        return false;
    }

    emit sigStartSession(sessionId);
    m_isRecording = true;
    return true;
}

void Q_RecordController::stopRecording() {
    if (!m_isRecording) return;

    m_captureProducer->stop();
    emit sigStopSession();
    m_isRecording = false;
}

bool Q_RecordController::isRecording() const { return m_isRecording; }

void Q_RecordController::onAudioDataAvailable(const QByteArray& buffer) {
    if (!m_isRecording) return;

    // update the UI, calculate the RMS
    float rms = calculateRMS(buffer);
    emit updateAmplitude(rms);

    // add data to the ring buffer for worker thread to save to the file
    size_t written = m_ringBuffer->write(buffer.constData(), buffer.size());
    LOG_TRACE("Written {} bytes to the ring buffer, current buffer size: {} / {}", written,
              m_ringBuffer->size(), m_ringBuffer->capacity());
    if (written < buffer.size()) {
        LOG_WARN("Failed to write all data to the ring buffer, only wrote {} bytes", written);
    }
}

void Q_RecordController::onStorageError(const QString& errorMessage) {
    LOG_ERROR("Failed to save audio data: {}", errorMessage.toStdString());
    emit errorOccurred(errorMessage);
}

void Q_RecordController::onSessionAborted() {
    LOG_ERROR("Session aborted");
    emit errorOccurred("Session aborted");
}

void Q_RecordController::onSessionFinalized(const std::filesystem::path& sessionPath) {
    LOG_INFO("Session finalized: {}", sessionPath.string());
    emit recordingFinished(sessionPath);
}

template <typename T>
constexpr double calNormFactor() {
    // for signed-number of n bits, the range is [-2^(n-1), 2^(n-1)-1]
    // so the normalized factor is 2^(n-1)
    // sizeof(T) * 8 is the number of bits
    return static_cast<double>(1LL << (sizeof(T) * 8 - 1));
}

float Q_RecordController::calculateRMS(const QByteArray& buffer) {
    const int16_t* samples = reinterpret_cast<const int16_t*>(buffer.constData());
    uint32_t sampleCnt = buffer.size() / sizeof(int16_t);

    if (sampleCnt <= 0) return 0.0f;

    double sumSquares = 0.0;
    for (size_t i = 0; i < sampleCnt; i += m_rmsStep) {
        int16_t sample = samples[i];
        double normalizedSample = sample / calNormFactor<int16_t>();
        sumSquares += normalizedSample * normalizedSample;
    }
    return static_cast<float>(std::sqrt(sumSquares / (sampleCnt / m_rmsStep)));
}
