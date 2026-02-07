#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QSignalSpy>

#include "thoth/Entity.h"
#include "thoth/Q_RecordController.h"

class QtEnv : public ::testing::Environment {
   public:
    void SetUp() override {
        int argc = 0;
        app = new QCoreApplication(argc, nullptr);
    }
    void TearDown() override { delete app; }

    QCoreApplication* app;
};

TEST(RecordControllerTest, StartRecordingShouldNotThrowException) {
    Q_RecordController controller;
    QSignalSpy spyError(&controller, &Q_RecordController::errorOccurred);

    Sentence sentence = {
        .id = "1",
        .text = "Hello, world!",
        .audioRange =
            TimeRange{
                .startMs = 0.0,
                .endMs = 1000.0,
            },
        .localAudioPath = std::filesystem::path("test.wav"),
    };
    RecordedSentence recordedSentence = {sentence, std::filesystem::path(), 0.0};

    try {
        controller.startRecording(recordedSentence);
    } catch (const std::exception& e) {
        FAIL() << "Exception thrown: " << e.what();
    } catch (...) {
        FAIL() << "startRecording threw unknown exception";
    }
    EXPECT_TRUE(controller.isRecording());
    EXPECT_EQ(controller.lastRecordingFilePath(), recordedSentence.localShadowingPath);
    EXPECT_EQ(controller.lastRecordingFilePath(), recordedSentence.localShadowingPath);

    if (spyError.count() > 0) {
        // error occurred
        auto args = spyError.takeFirst();
        std::cout << "Error occurred: " << args.at(0).toString().toStdString() << std::endl;
    }
}