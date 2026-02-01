#include "thoth/Q_UserAudioRecorder.h"

#include <QDebug>

#include "internal/Entity.h"
#include "thoth/ConfigKey.h"
#include "thoth/ConfigStore.h"
#include "thoth/Logger.h"

Q_UserAudioRecorder::Q_UserAudioRecorder(QObject* parent) : QObject(parent) { initAudio(); }

Q_UserAudioRecorder::~Q_UserAudioRecorder() { stopRecording(); }

uint16_t QAudioSampleFormatToBits(QAudioFormat::SampleFormat format) {
    switch (format) {
        case QAudioFormat::SampleFormat::UInt8:
            return 8;
        case QAudioFormat::SampleFormat::Int16:
            return 16;
        case QAudioFormat::SampleFormat::Int32:
            return 32;
        case QAudioFormat::SampleFormat::Float:
            return 64;
        default:
            return 16;
    }
}

void Q_UserAudioRecorder::initAudio() {
    // By default, use 16kHz, Mono, 16-bit, which is compatiable to OpenAI Whisper
    m_audioFormat.setSampleRate(
        ConfigStore::instance()
            .getValue<uint32_t>(thoth::config::KEY_AUDIO_RECORDER_SAMPLE_RATE)
            .value_or(thoth::config::DEFAULT_AUDIO_RECORDER_SAMPLE_RATE));
    m_audioFormat.setChannelCount(
        ConfigStore::instance()
            .getValue<uint16_t>(thoth::config::KEY_AUDIO_RECORDER_CHANNELS)
            .value_or(thoth::config::DEFAULT_AUDIO_RECORDER_CHANNELS));
    m_audioFormat.setSampleFormat(
        [](uint16_t value) -> QAudioFormat::SampleFormat {
            if (value == 8) return QAudioFormat::SampleFormat::UInt8;
            if (value == 16) return QAudioFormat::SampleFormat::Int16;
            if (value == 32) return QAudioFormat::SampleFormat::Int32;
            if (value == 64) return QAudioFormat::SampleFormat::Float;
            return QAudioFormat::SampleFormat::Int16;
        }(ConfigStore::instance()
                               .getValue<uint16_t>(thoth::config::KEY_AUDIO_RECORDER_SAMPLE_FORMAT)
                               .value_or(thoth::config::DEFAULT_AUDIO_RECORDER_SAMPLE_FORMAT)));

    QAudioDevice defaultDevice = QMediaDevices::defaultAudioOutput();
    if (!defaultDevice.isFormatSupported(m_audioFormat)) {
        LOG_ERROR(
            "Default audio output device does not support the requested format, trying the nearest "
            "format.");
        m_audioFormat = defaultDevice.preferredFormat();
        LOG_WARN("Using the nearest format: {}kHz, {}ch, {}bit", m_audioFormat.sampleRate(),
                 m_audioFormat.channelCount(),
                 QAudioSampleFormatToBits(m_audioFormat.sampleFormat()));
    }

    m_audioSource = std::make_unique<QAudioSource>(defaultDevice, m_audioFormat);
}

bool Q_UserAudioRecorder::startRecording(const std::filesystem::path& filePath) {
    if (m_audioSource->state() == QAudio::ActiveState) {
        return false;
    }

    m_audioFile = std::make_unique<QFile>(QString::fromStdString(filePath.string()));
    if (!m_audioFile->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        LOG_ERROR("Failed to open file {} for recording", filePath.string());
        emit errorOccurred("Failed to create recording file");
        return false;
    }

    QByteArray placeholder(44, 0);
    m_audioFile->write(placeholder);
    m_totalBytes = 0;

    m_audioStream = m_audioSource->start();
    if (m_audioSource->error() != QAudio::NoError) {
        LOG_ERROR("Failed to start audio recording: {}", static_cast<int>(m_audioSource->error()));
        emit errorOccurred("Failed to start audio recording: ");
        return false;
    }

    connect(m_audioStream, &QIODevice::readyRead, this, &Q_UserAudioRecorder::onReadyRecord);
    return true;
}

void Q_UserAudioRecorder::stopRecording() {
    if (!m_audioSource || m_audioSource->state() == QAudio::StoppedState) {
        return;
    }

    m_audioSource->stop();
    if (m_audioStream) {
        // disconnect and avoid remaining data envoking the slot
        disconnect(m_audioStream, &QIODevice::readyRead, this, nullptr);
        m_audioStream = nullptr;
    }

    // back-writing the actual size to WAV Header
    if (m_audioFile && m_audioFile->isOpen()) {
        m_audioFile->seek(0);  // back to the beginning of the file

        WAVHeader wavHeader =
            WAVHeader::create(m_audioFormat.sampleRate(), m_audioFormat.channelCount(),
                              QAudioSampleFormatToBits(m_audioFormat.sampleFormat()), m_totalBytes);
        m_audioFile->write(reinterpret_cast<const char*>(&wavHeader), sizeof(WAVHeader));
        m_audioFile->close();
        LOG_INFO("Recording stopped and saved to {}", m_audioFile->fileName().toStdString());
    } else {
        LOG_ERROR("Failed to write WAV header to file");
        emit errorOccurred("Failed to write WAV header to file");
    }
}

bool Q_UserAudioRecorder::isRecording() const {
    return m_audioSource && m_audioSource->state() == QAudio::ActiveState;
}

std::optional<std::filesystem::path> Q_UserAudioRecorder::lastRecordingFilePath() const {
    if (m_audioFile && m_audioFile->isOpen()) {
        return std::filesystem::path(m_audioFile->fileName().toStdString());
    }
    return std::nullopt;
}

void Q_UserAudioRecorder::onReadyRecord() {
    if (!m_audioStream || !m_audioFile) return;

    QByteArray buffer = m_audioStream->readAll();
    if (buffer.isEmpty()) return;

    qint64 written = m_audioFile->write(buffer);
    if (written > 0) {
        m_totalBytes += written;
    }

    float level = calculateRMS(buffer);
    emit updateAmplitude(level);
}

template <typename T>
constexpr double calNormFactor() {
    // for signed-number of n bits, the range is [-2^(n-1), 2^(n-1)-1]
    // so the normalized factor is 2^(n-1)
    // sizeof(T) * 8 is the number of bits
    return static_cast<double>(1LL << (sizeof(T) * 8 - 1));
}

float Q_UserAudioRecorder::calculateRMS(const QByteArray& buffer) {
    const int16_t* samples = reinterpret_cast<const int16_t*>(buffer.constData());
    int sampleCnt = buffer.size() / sizeof(int16_t);

    if (sampleCnt <= 0) return 0.0f;

    double sumSquares = 0.0;
    // step is 8 by default, for calculation efficiency
    uint16_t step = ConfigStore::instance()
                        .getValue<uint16_t>(thoth::config::KEY_AUDIO_RECORDER_BUFFER_SIZE)
                        .value_or(thoth::config::DEFAULT_AUDIO_RECORDER_BUFFER_SIZE);
    for (size_t i = 0; i < sampleCnt; i += step) {
        int16_t sample = samples[i];
        double normalizedSample = sample / calNormFactor<int16_t>();
        sumSquares += normalizedSample * normalizedSample;
    }
    return static_cast<float>(std::sqrt(sumSquares / (sampleCnt / step)));
}