#include <QTimer>

#include "internal/InternalEntity.h"
#include "internal/LockFreeRingBuffer.h"
#include "internal/Q_AudioStorage.h"
#include "internal/Timer.h"
#include "internal/Utils.h"
#include "thoth/ConfigKey.h"
#include "thoth/ConfigStore.h"
#include "thoth/Logger.h"

AudioFileStreamSaver::AudioFileStreamSaver(LockFreeRingBuffer* buffer, const QAudioFormat& format,
                                           std::optional<std::filesystem::path> rootDir,
                                           QObject* parent)
    : Q_AudioStreamConsumer(parent), m_dataSource(buffer), m_format(format) {
    if (rootDir) {
        m_rootDir = *rootDir;
    } else {
        m_rootDir = ConfigStore::instance().getCacheDir() / "record" /
                    thoth::internal::getCurrentTimestamp();
    }
    if (!std::filesystem::exists(m_rootDir)) {
        std::filesystem::create_directories(m_rootDir);
    }
    m_timer = new QTimer(this);
    m_timer->setInterval(DRAIN_INTERVAL_MS);
    connect(m_timer, &QTimer::timeout, this, &AudioFileStreamSaver::_drainBufferToFile);
}

AudioFileStreamSaver::~AudioFileStreamSaver() {
    m_timer->stop();
    delete m_timer;
}

void AudioFileStreamSaver::startSession(const std::string& sessionId) {
    if (m_file && m_file->isOpen()) {
        LOG_ERROR("Session already started");
        abortSession();
    }
    m_sessionId = QString::fromStdString(sessionId);
    auto path = m_rootDir / (m_sessionId.toStdString() + DEFAULT_FILE_EXTENSION.data());
    QString qPath;
#ifdef _WIN32
    qPath = QString::fromStdWString(path.wstring());
#else
    qPath = QString::fromStdString(path.string());
#endif
    m_file = new QFile(qPath);
    if (!m_file->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        LOG_ERROR("Failed to open file for session: {}", m_sessionId.toStdString());
        emit errorOccurred(QString("Failed to create file %1").arg(m_file->fileName()));
        return;
    }

    // WAV Placeholder
    m_totalBytes = 0;
    QByteArray placeholder(sizeof(WAVHeader), 0);
    m_file->write(placeholder.data(), placeholder.size());

    m_dataSource->clear();
    m_timer->start();
}

void AudioFileStreamSaver::finalizeSession() {
    m_timer->stop();
    _drainBufferToFile();

    // back-writing the actual size to WAV Header
    if (m_file && m_file->isOpen()) {
        m_file->seek(0);  // back to the beginning of the file
        WAVHeader wavHeader = WAVHeader::create(
            m_format.sampleRate(), m_format.channelCount(),
            thoth::internal::QAudioSampleFormatToBits(m_format.sampleFormat()), m_totalBytes);
        m_file->write(reinterpret_cast<const char*>(&wavHeader), sizeof(WAVHeader));

        QString fileName = m_file->fileName();
        m_file->close();
        emit sessionFinalized(fileName.toStdString());
    } else {
        LOG_ERROR("Failed to write WAV header to file");
        emit errorOccurred(
            QString("Failed to write WAV header to file %1").arg(m_file->fileName()));
    }
}

void AudioFileStreamSaver::abortSession() {
    m_timer->stop();
    if (m_file && m_file->isOpen()) {
        m_file->close();
        m_file->remove();
        emit sessionAborted();
    } else {
        LOG_ERROR("Failed to abort session");
        emit errorOccurred(QString("Failed to abort session %1").arg(m_file->fileName()));
    }
}

void AudioFileStreamSaver::_drainBufferToFile() {
    if (!m_file || !m_file->isOpen()) {
        return;
    }

    while (true) {
        size_t readBytes = m_dataSource->read(m_ioBuffer.data(), m_ioBuffer.size());
        if (readBytes == 0) break;

        uint64_t writtenBytes = m_file->write(m_ioBuffer.data(), readBytes);
        if (writtenBytes > 0) {
            m_totalBytes += writtenBytes;
        } else {
            LOG_ERROR("Failed to write to file {}", m_file->fileName().toStdString());
            emit errorOccurred(QString("Failed to write to file %1").arg(m_file->fileName()));
            m_timer->stop();
            break;
        }
    }
}