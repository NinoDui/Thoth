#pragma once

#include <cstdint>
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

}  // namespace thoth
