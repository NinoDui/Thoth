#include "Q_AudioDecoder.h"

#include <QAudioDecoder>
#include <QAudioFormat>
#include <QEventLoop>
#include <QUrl>
#include <stdexcept>

Q_AudioDecoder::Q_AudioDecoder(QObject* parent) : QObject(parent) {}

std::vector<float> Q_AudioDecoder::decodeToMono16kFloat(const std::string& path) {
    QAudioDecoder decoder;

    QAudioFormat format;
    format.setSampleRate(16000);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Float);
    decoder.setAudioFormat(format);

    std::vector<float> samples;
    QEventLoop loop;

    QObject::connect(&decoder, &QAudioDecoder::bufferReady, [&]() {
        auto buffer = decoder.read();
        if (!buffer.isValid()) {
            return;
        }
        const float* data = buffer.constData<float>();
        qsizetype count = buffer.sampleCount();
        samples.insert(samples.end(), data, data + count);
    });

    QObject::connect(&decoder, &QAudioDecoder::finished, &loop, &QEventLoop::quit);
    void (QAudioDecoder::*errSignal)(QAudioDecoder::Error) = &QAudioDecoder::error;
    QObject::connect(&decoder, errSignal, &loop, &QEventLoop::quit);

    decoder.setSource(QUrl::fromLocalFile(QString::fromStdString(path)));
    decoder.start();
    loop.exec();

    if (decoder.error() != QAudioDecoder::NoError) {
        throw std::runtime_error("Audio decode failed: " + decoder.errorString().toStdString());
    }

    if (samples.empty()) {
        throw std::runtime_error("Audio decode produced no samples");
    }

    return samples;
}
