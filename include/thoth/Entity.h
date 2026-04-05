#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

struct TimeRange {
    double startMs = 0.0;
    double endMs = 0.0;
};

struct Sentence {
    std::string id;
    std::string text;

    // For pure text input -> nullptr;
    // For audio input -> the time range of the audio in the sentence;
    std::optional<TimeRange> audioRange;

    // Cache Audio File Path
    std::filesystem::path localAudioPath;
};

struct RecordedSentence : Sentence {
    std::filesystem::path localShadowingPath;
    double shadowingScore = 0.0;
    std::optional<std::string> transcribedText;
};

class Session {
   public:
    ~Session() = default;

    std::string id;
    std::string title;
    std::vector<Sentence> sentences;
    std::optional<std::vector<RecordedSentence>> recordedSentences;

    std::filesystem::path inputMediaPath;
};