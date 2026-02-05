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

    double score = 0.0;
};

class Session {
   public:
    ~Session() = default;

    std::string id;
    std::string title;
    std::vector<Sentence> sentences;

    std::filesystem::path inputMediaPath;
};