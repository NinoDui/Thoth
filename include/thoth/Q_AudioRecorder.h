#pragma once

#include <QAudioSource>
#include <QFile>
#include <QMediaDevices>
#include <QObject>
#include <filesystem>
#include <memory>
#include <optional>

struct WAVHeader;

class Q_AudioRecorder : public QObject {
    Q_OBJECT
   public:
    explicit Q_AudioRecorder(QObject* parent = nullptr);
    ~Q_AudioRecorder();

    bool startRecording(const std::filesystem::path& filePath);
    void stopRecording();
    bool isRecording() const;

    std::optional<std::filesystem::path> lastRecordingFilePath() const;

   signals:
    void updateAmplitude(float level);
    void errorOccurred(const QString& errorMessage);

   private slots:
    void onReadyRecord();

   private:
    void initAudio();
    float calculateRMS(const QByteArray& buffer);

    std::unique_ptr<QAudioSource> m_audioSource;
    std::unique_ptr<QFile> m_audioFile;

    // observer pointer maintained by m_audioSource
    QIODevice* m_audioStream = nullptr;

    QAudioFormat m_audioFormat;
    uint32_t m_totalBytes = 0;
};