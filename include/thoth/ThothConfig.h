#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

namespace thoth {

struct AudioRecorderConfig {
    uint32_t sampleRate = 16000;
    uint16_t channels = 1;
    uint16_t sampleFormatBits = 16;  // 8=UInt8, 16=Int16, 32=Int32, 64=Float
    uint16_t rmsStep = 8;
    uint32_t bufferSize = 1024;
};

struct WhisperConfig {
    std::string modelPath = "models/ggml-base.en.bin";
    std::string language = "en";
};

struct GoogleTTSConfig {
    std::string languageCode;
    std::string voiceName;
    std::string audioEncoding;
};

struct LogConfig {
    std::string level;
    std::string pattern;
    bool toConsole;
    bool toFile;
    std::filesystem::path logDir;
};

}  // namespace thoth
