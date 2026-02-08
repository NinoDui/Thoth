#pragma once

#include <cstdint>

#pragma pack(push, 1)  // structure packing, force alignment to 1 byte

struct WAVHeader {
    char riff[4] = {'R', 'I', 'F', 'F'};
    uint32_t overallSize;  // size starting from here exclusively
    char wave[4] = {'W', 'A', 'V', 'E'};
    char fmt[4] = {'f', 'm', 't', ' '};
    uint32_t fmtLength = 16;
    uint16_t fmtType = 1;  // PCM
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;  // bytes used in playing for 1s, for example, 16kHz 16bits PCM is 16,000 * 2
                        // bytes = 32,000 bytes/s
    uint16_t blockAlign;  // Frame size in bytes, for 16bits PCM is 16 * 1 / 8 = 2 bytes
    uint16_t bitsPerSample;
    char data[4] = {'d', 'a', 't', 'a'};
    uint32_t dataSize;

    static WAVHeader create(uint32_t sampleRate, uint16_t numChannels, uint16_t bitsPerSample,
                            uint32_t dataSize);
};

// fail fast!
static_assert(sizeof(WAVHeader) == 44, "WAVHeader size must be 44 bytes");

#pragma pack(pop)