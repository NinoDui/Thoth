#include <gtest/gtest.h>

#include <QAudioFormat>
#include <filesystem>
#include <fstream>

#include "internal/LockFreeRingBuffer.h"
#include "internal/Q_AudioStorage.h"

namespace fs = std::filesystem;

namespace {
QAudioFormat TestFormat() {
    QAudioFormat format;
    format.setSampleRate(16000);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);
    return format;
}
}  // namespace

TEST(StreamAudioStorageTest, SameSessionIdReplacesExistingRecordingFile) {
    const fs::path rootDir = fs::temp_directory_path() / "ThothTests" / "record-replace";
    fs::remove_all(rootDir);
    fs::create_directories(rootDir);

    LockFreeRingBuffer ringBuffer(4096);
    AudioFileStreamSaver saver(&ringBuffer, TestFormat(), rootDir);

    saver.startSession("sentence-0");
    saver.finalizeSession();

    const fs::path recordingPath = rootDir / "sentence-0.wav";
    ASSERT_TRUE(fs::exists(recordingPath));

    {
        std::ofstream oldFile(recordingPath, std::ios::binary | std::ios::trunc);
        oldFile << std::string(128, 'x');
    }
    ASSERT_EQ(fs::file_size(recordingPath), 128);

    saver.startSession("sentence-0");
    saver.finalizeSession();

    ASSERT_TRUE(fs::exists(recordingPath));
    EXPECT_EQ(fs::file_size(recordingPath), 44);

    fs::remove_all(rootDir);
}
