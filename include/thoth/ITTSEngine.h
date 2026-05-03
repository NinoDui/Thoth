#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace thoth {

struct TTSAudioResult {
    std::vector<uint8_t> audioData;
    std::string format;  // "mp3", "wav", "pcm16"
    uint32_t sampleRate = 0;
};

class ITTSEngine {
   public:
    virtual ~ITTSEngine() = default;

    virtual TTSAudioResult synthesize(const std::string& text) = 0;
    virtual std::string engineName() const = 0;
    virtual std::string cacheIdentity() const = 0;
    virtual bool isAvailable() const = 0;
};

}  // namespace thoth
