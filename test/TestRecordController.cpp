#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QSignalSpy>

#include "internal/LockFreeRingBuffer.h"
#include "internal/Q_AudioCapture.h"
#include "internal/Q_AudioStorage.h"
#include "thoth/Q_RecordController.h"

class MockAudioCaptureProducer : public Q_AudioCaptureProducer {
   public:
    MOCK_METHOD(bool, start, (), (override));
    MOCK_METHOD(void, stop, (), (override));
    MOCK_METHOD(QAudioFormat, format, (), (const, override));

    void simulateDataAvailable(const QByteArray& data) { emit audioDataAvailable(data); }
};

class MockAudioFileStreamSaver : public AudioFileStreamSaver {
   public:
    MockAudioFileStreamSaver(LockFreeRingBuffer* buffer, const QAudioFormat& format)
        : AudioFileStreamSaver(buffer, format) {}
    MOCK_METHOD(void, startSession, (const std::string& sessionId), (override));
    MOCK_METHOD(void, finalizeSession, (), (override));
    MOCK_METHOD(void, abortSession, (), (override));

    void simulateSessionFinalized(const std::filesystem::path& sessionPath) {
        emit sessionFinalized(sessionPath);
    }

    void simulateSessionAborted() { emit sessionAborted(); }
    void simulateErrorOccurred(const QString& errorMessage) { emit errorOccurred(errorMessage); }
};

class RecordControllerTest : public ::testing::Test {
   protected:
    void SetUp() override {
        auto captureProducerPtr = std::make_unique<MockAudioCaptureProducer>();
        mockCaptureProducer = captureProducerPtr.get();
        auto ringBufferPtr = std::make_unique<LockFreeRingBuffer>(4096);
        ringBuffer = ringBufferPtr.get();
        mockStorage =
            new MockAudioFileStreamSaver(ringBufferPtr.get(), mockCaptureProducer->format());

        controller = std::make_unique<Q_RecordController>(std::move(captureProducerPtr),
                                                          mockStorage, std::move(ringBufferPtr));
    }

    void TearDown() override { controller.reset(); }

    std::unique_ptr<Q_RecordController> controller;
    MockAudioCaptureProducer* mockCaptureProducer;
    MockAudioFileStreamSaver* mockStorage;
    LockFreeRingBuffer* ringBuffer;
};

TEST_F(RecordControllerTest, StartRecordingSuccessFlow) {
    std::string sessionId = "StartRecordingSuccessFlow";

    {
        ::testing::InSequence s;
        EXPECT_CALL(*mockCaptureProducer, start()).WillOnce(::testing::Return(true));
        EXPECT_CALL(*mockStorage, startSession(sessionId)).Times(1);
    }

    // storage is working on another thread, so we "spy" the behavior of controller
    QSignalSpy spyStart(controller.get(), &Q_RecordController::sigStartSession);
    bool res = controller->startRecording(sessionId);

    EXPECT_TRUE(res);
    EXPECT_EQ(spyStart.count(), 1);
    EXPECT_EQ(spyStart.takeFirst().at(0).value<std::string>(), sessionId);
    EXPECT_TRUE(controller->isRecording());
}

TEST_F(RecordControllerTest, AudioDataFlowWritesToBuffer) {
    EXPECT_CALL(*mockCaptureProducer, start()).WillOnce(::testing::Return(true));
    auto sessionId = "AudioDataFlowWritesToBuffer";
    controller->startRecording(sessionId);

    QByteArray fakeData(100, 0x11);
    QSignalSpy spyAmp(controller.get(), &Q_RecordController::updateAmplitude);

    mockCaptureProducer->simulateDataAvailable(fakeData);

    // UI receives the RMS update signal
    EXPECT_EQ(spyAmp.count(), 1);
    EXPECT_EQ(ringBuffer->size(), fakeData.size());
}

TEST_F(RecordControllerTest, StopRecordingSuccessFlow) {
    EXPECT_CALL(*mockCaptureProducer, start()).WillOnce(::testing::Return(true));
    auto sessionID = "StopRecordingSuccessFlow";
    controller->startRecording(sessionID);

    {
        ::testing::InSequence s;
        EXPECT_CALL(*mockCaptureProducer, stop()).Times(1);
        EXPECT_CALL(*mockStorage, finalizeSession()).Times(1);
    }
    QSignalSpy spyStop(controller.get(), &Q_RecordController::sigStopSession);
    controller->stopRecording();

    spyStop.wait(500);

    EXPECT_FALSE(controller->isRecording());
    EXPECT_EQ(spyStop.count(), 1);
}

TEST_F(RecordControllerTest, StorageFinalizeEmitsFinished) {
    QSignalSpy spyFinished(controller.get(), &Q_RecordController::recordingFinished);
    emit mockStorage->sessionFinalized(std::filesystem::path("test.wav"));
    // storage is working on io-worker thread, while controller lies in the main thread
    // their communication relies on QueuedConnection(by default)
    // Force the core app in main thread to hang test and handle the signal in the event queue
    QCoreApplication::processEvents();

    EXPECT_EQ(spyFinished.count(), 1);
    EXPECT_EQ(spyFinished.takeFirst().at(0).value<std::filesystem::path>(),
              std::filesystem::path("test.wav"));
}
