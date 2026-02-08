#include <QAudioSource>
#include <QMediaDevices>

#include "internal/Q_AudioCapture.h"
#include "internal/Utils.h"
#include "thoth/ConfigKey.h"
#include "thoth/ConfigStore.h"
#include "thoth/Logger.h"

Q_AudioCaptureProducer::Q_AudioCaptureProducer(QObject* parent) : QObject(parent) { initAudio(); }

Q_AudioCaptureProducer::~Q_AudioCaptureProducer() { stop(); }

bool Q_AudioCaptureProducer::start() {
    if (!m_audioSource || m_audioSource->state() == QAudio::ActiveState) {
        return false;
    }
    if (!m_device) {
        return false;
    }

    connect(m_device, &QIODevice::readyRead, this, &Q_AudioCaptureProducer::_onReadyRead);
    return true;
}

void Q_AudioCaptureProducer::stop() {
    if (!m_audioSource || m_audioSource->state() == QAudio::StoppedState) {
        return;
    }
    if (m_audioSource) {
        m_audioSource->stop();
    }
    if (m_device) {
        disconnect(m_device, &QIODevice::readyRead, this, nullptr);
    }
    m_device = nullptr;
}

QAudioFormat Q_AudioCaptureProducer::format() const { return m_format; }

void Q_AudioCaptureProducer::_onReadyRead() {
    if (!m_device) {
        LOG_ERROR("Audio device is not set");
        return;
    }
    QByteArray buffer = m_device->readAll();
    if (!buffer.isEmpty()) {
        emit audioDataAvailable(buffer);
    } else {
        LOG_WARN("No audio data available from IO Device");
    }
}

void Q_AudioCaptureProducer::initAudio() {
    // By default, use 16kHz, Mono, 16-bit, which is compatiable to OpenAI Whisper
    m_format.setSampleRate(ConfigStore::instance()
                               .getValue<uint32_t>(thoth::config::KEY_AUDIO_RECORDER_SAMPLE_RATE)
                               .value_or(thoth::config::DEFAULT_AUDIO_RECORDER_SAMPLE_RATE));
    m_format.setChannelCount(ConfigStore::instance()
                                 .getValue<uint16_t>(thoth::config::KEY_AUDIO_RECORDER_CHANNELS)
                                 .value_or(thoth::config::DEFAULT_AUDIO_RECORDER_CHANNELS));
    m_format.setSampleFormat(
        [](uint16_t value) -> QAudioFormat::SampleFormat {
            if (value == 8) return QAudioFormat::SampleFormat::UInt8;
            if (value == 16) return QAudioFormat::SampleFormat::Int16;
            if (value == 32) return QAudioFormat::SampleFormat::Int32;
            if (value == 64) return QAudioFormat::SampleFormat::Float;
            return QAudioFormat::SampleFormat::Int16;
        }(ConfigStore::instance()
                               .getValue<uint16_t>(thoth::config::KEY_AUDIO_RECORDER_SAMPLE_FORMAT)
                               .value_or(thoth::config::DEFAULT_AUDIO_RECORDER_SAMPLE_FORMAT)));

    QAudioDevice defaultDevice = QMediaDevices::defaultAudioInput();
    if (!defaultDevice.isFormatSupported(m_format)) {
        LOG_ERROR(
            "Default audio input device does not support the requested format, trying the nearest "
            "format.");
        m_format = defaultDevice.preferredFormat();
        LOG_WARN("Using the nearest format: {}kHz, {}ch, {}bit", m_format.sampleRate(),
                 m_format.channelCount(),
                 thoth::internal::QAudioSampleFormatToBits(m_format.sampleFormat()));
    }

    m_audioSource = std::make_unique<QAudioSource>(defaultDevice, m_format);
}
