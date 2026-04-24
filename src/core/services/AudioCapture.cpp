#include <QAudioSource>
#include <QMediaDevices>

#include "internal/Q_AudioCapture.h"
#include "internal/Utils.h"
#include "thoth/Logger.h"

Q_AudioCaptureProducer::Q_AudioCaptureProducer(QObject* parent) : QObject(parent) {
    m_format.setSampleRate(16000);
    m_format.setChannelCount(1);
    m_format.setSampleFormat(QAudioFormat::Int16);
    initAudio();
}

Q_AudioCaptureProducer::Q_AudioCaptureProducer(const QAudioFormat& format, QObject* parent)
    : QObject(parent), m_format(format) {
    initAudio();
}

Q_AudioCaptureProducer::~Q_AudioCaptureProducer() { stop(); }

bool Q_AudioCaptureProducer::start() {
    if (!m_audioSource || m_audioSource->state() == QAudio::ActiveState) {
        LOG_DEBUG("Audio source is not ready for recording, current state is {}",
                  static_cast<int>(m_audioSource->state()));
        return false;
    }
    if (!m_device) {
        LOG_INFO("Starting audio capture, no device found, creating a new one");
        m_device = m_audioSource->start();
        if (!m_device) {
            LOG_ERROR("Failed to start audio capture, no device found");
            emit errorOccurred("Failed to start audio capture, no device found");
            return false;
        }

        connect(m_device, &QIODevice::readyRead, this, &Q_AudioCaptureProducer::_onReadyRead);
    }

    return true;
}

void Q_AudioCaptureProducer::stop() {
    if (!m_audioSource) {
        LOG_DEBUG("Audio source is not set in Audio Capture Producer, skip the stop operation.");
        return;
    }
    if (m_audioSource || m_audioSource->state() != QAudio::StoppedState) {
        m_audioSource->stop();
    }
    if (m_device) {
        disconnect(m_device, &QIODevice::readyRead, this, nullptr);
        m_device = nullptr;
    }
}

QAudioFormat Q_AudioCaptureProducer::format() const { return m_format; }

void Q_AudioCaptureProducer::_onReadyRead() {
    if (!m_device) {
        LOG_ERROR("Audio device is not set");
        return;
    }

    // FOR DEBUG ONLY
    if (m_audioSource && m_audioSource->error() != QAudio::NoError) {
        LOG_ERROR("Audio source error: {}", static_cast<int>(m_audioSource->error()));
        emit errorOccurred(
            QString("Audio source error: %1").arg(static_cast<int>(m_audioSource->error())));
        return;
    }

    QByteArray buffer = m_device->readAll();
    if (!buffer.isEmpty()) {
        emit audioDataAvailable(buffer);
        LOG_TRACE("Audio data available from IO Device, size: {}", buffer.size());
    } else {
        LOG_TRACE("No audio data available from IO Device");
    }
}

void Q_AudioCaptureProducer::initAudio() {
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
