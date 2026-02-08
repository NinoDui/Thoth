#pragma once

#include <QAudioFormat>
#include <QAudioSource>
#include <QIODevice>
#include <QObject>

/*
    The AudioCaptureService is the interface for all audio capture devices.
*/
class IAudioCaptureService {
   public:
    virtual ~IAudioCaptureService() = default;

    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual QAudioFormat format() const = 0;
};

class Q_AudioCaptureProducer : public QObject, public IAudioCaptureService {
    Q_OBJECT
   public:
    Q_AudioCaptureProducer(QObject* parent = nullptr);
    ~Q_AudioCaptureProducer() override;

    bool start() override;
    void stop() override;
    QAudioFormat format() const override;

   signals:
    void audioDataAvailable(const QByteArray& data);
    void errorOccurred(const QString& errorMessage);

   private:
    void initAudio();
    void _onReadyRead();

    std::unique_ptr<QAudioSource> m_audioSource;
    QIODevice* m_device = nullptr;
    QAudioFormat m_format;
};