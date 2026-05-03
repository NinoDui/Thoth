#pragma once

#include <QAudioFormat>
#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QObject>
#include <QTimer>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>

class LockFreeRingBuffer;

class IStreamStorage {
   public:
    virtual ~IStreamStorage() = default;

    virtual void startSession(const std::string& sessionId) = 0;
    virtual void finalizeSession() = 0;
    virtual void abortSession() = 0;
};

class Q_AudioStreamConsumer : public QObject, public IStreamStorage {
    Q_OBJECT
   public:
    explicit Q_AudioStreamConsumer(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~Q_AudioStreamConsumer() override = default;

   public slots:
    virtual void startSession(const std::string& sessionId) override = 0;
    virtual void finalizeSession() override = 0;
    virtual void abortSession() override = 0;

   signals:
    void sessionFinalized(const std::filesystem::path& sessionPath);
    void sessionAborted();
    void errorOccurred(const QString& errorMessage);
};

class AudioFileStreamSaver : public Q_AudioStreamConsumer {
    Q_OBJECT
   public:
    explicit AudioFileStreamSaver(LockFreeRingBuffer* buffer, const QAudioFormat& format,
                                  std::optional<std::filesystem::path> rootDir = std::nullopt,
                                  QObject* parent = nullptr);
    ~AudioFileStreamSaver() override;

   public slots:
    void startSession(const std::string& sessionId) override;
    void finalizeSession() override;
    void abortSession() override;

   private:
    void _resolveRootDir();
    void _drainBufferToFile();
    void _stop();

    LockFreeRingBuffer* m_dataSource = nullptr;
    std::filesystem::path m_rootDir;
    bool m_hasExplicitRootDir = false;

    std::unique_ptr<QFile> m_file;
    QString m_sessionId;
    uint64_t m_totalBytes = 0;
    QTimer* m_timer;
    QAudioFormat m_format;
    std::vector<char> m_ioBuffer;

    static constexpr size_t IO_BUFFER_SIZE = 4096;
    static constexpr size_t DRAIN_INTERVAL_MS = 30;
    static constexpr std::string_view DEFAULT_FILE_EXTENSION = ".wav";
};
