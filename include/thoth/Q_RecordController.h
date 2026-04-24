#pragma once

#include <QAudioFormat>
#include <QObject>
#include <QThread>
#include <filesystem>
#include <memory>
#include <optional>

class LockFreeRingBuffer;
class Q_AudioCaptureProducer;
class AudioFileStreamSaver;

class Q_RecordController : public QObject {
    Q_OBJECT
   public:
    explicit Q_RecordController(const QAudioFormat& format, uint16_t rmsStep = 8,
                                QObject* parent = nullptr);
    explicit Q_RecordController(std::unique_ptr<Q_AudioCaptureProducer> captureProducer,
                                AudioFileStreamSaver* streamSaver,
                                std::unique_ptr<LockFreeRingBuffer> ringBuffer,
                                uint16_t rmsStep = 8, QObject* parent = nullptr);
    ~Q_RecordController();

    bool startRecording(const std::string& sessionId);
    void stopRecording();
    bool isRecording() const;

   signals:
    // external to ThothApp - UI signals
    void updateAmplitude(float level);
    void recordingFinished(const std::filesystem::path& sessionPath);
    void errorOccurred(const QString& errorMessage);

    // internal controlling signals, to IO Worker thread
    void sigStartSession(const std::string& sessionId);
    void sigStopSession();
    void sigAbortSession();

   private slots:
    // for audio capture device
    void onAudioDataAvailable(const QByteArray& buffer);

    // for IO Worker thread
    void onStorageError(const QString& errorMessage);
    void onSessionAborted();
    void onSessionFinalized(const std::filesystem::path& sessionPath);

   private:
    float calculateRMS(const QByteArray& buffer);
    void setupConnections();

    bool m_isRecording = false;
    uint16_t m_rmsStep = 8;

    std::unique_ptr<LockFreeRingBuffer> m_ringBuffer;
    std::unique_ptr<Q_AudioCaptureProducer> m_captureProducer;

    // IO Worker
    AudioFileStreamSaver* m_streamSaver;
    QThread* m_ioWorkerThread;

    static constexpr size_t DEFAULT_RING_BUFFER_SIZE = 1024 * 1024;  // 1MB
};
