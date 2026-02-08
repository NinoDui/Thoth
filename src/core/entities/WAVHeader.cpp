#include "internal/InternalEntity.h"

WAVHeader WAVHeader::create(uint32_t sampleRate, uint16_t numChannels, uint16_t bitsPerSample,
                            uint32_t dataSize) {
    WAVHeader h;
    h.numChannels = numChannels;
    h.sampleRate = sampleRate;
    h.bitsPerSample = bitsPerSample;
    h.dataSize = dataSize;
    h.blockAlign = (numChannels * bitsPerSample) / 8;
    h.byteRate = sampleRate * h.blockAlign;
    h.overallSize = h.dataSize + 44 - 8;
    return h;
}