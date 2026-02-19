#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstring>
#include <sstream>
#include <vector>

#include "internal/InternalEntity.h"

class WAVTest : public ::testing::Test {
   protected:
    // Helper to create a valid WAV header
    WAVHeader createValidHeader(uint32_t sampleRate = 16000, uint16_t numChannels = 1,
                                uint16_t bitsPerSample = 16, uint32_t dataSize = 100) {
        return WAVHeader::create(sampleRate, numChannels, bitsPerSample, dataSize);
    }

    // Helper to create a stream from header and data
    std::stringstream createStream(const WAVHeader& header, const std::vector<char>& data) {
        std::stringstream ss;
        ss.write(reinterpret_cast<const char*>(&header), sizeof(WAVHeader));
        if (!data.empty()) {
            ss.write(data.data(), data.size());
        }
        return ss;
    }
};

TEST_F(WAVTest, CreateHeaderMatchesExpectedValues) {
    auto h = WAVHeader::create(16000, 1, 16, 32000);
    EXPECT_EQ(h.sampleRate, 16000);
    EXPECT_EQ(h.numChannels, 1);
    EXPECT_EQ(h.bitsPerSample, 16);
    EXPECT_EQ(h.dataSize, 32000);
    EXPECT_EQ(h.blockAlign, 2);    // 1 * 16 / 8
    EXPECT_EQ(h.byteRate, 32000);  // 16000 * 2
    EXPECT_TRUE(h.isValid());
}

TEST_F(WAVTest, ReadHeaderFromStream) {
    auto h = createValidHeader();
    std::stringstream ss;
    ss.write(reinterpret_cast<const char*>(&h), sizeof(WAVHeader));

    WAVHeader readHeader = WAVHeader::read(ss);
    EXPECT_EQ(readHeader.sampleRate, h.sampleRate);
    EXPECT_EQ(readHeader.dataSize, h.dataSize);
    EXPECT_TRUE(readHeader.isValid());
}

TEST_F(WAVTest, DecodeValidWAV16Bit) {
    // 16-bit PCM: 1 channel, 16kHz.
    uint16_t bits = 16;
    uint32_t sampleRate = 16000;
    uint16_t channels = 1;

    // Data: 2 samples. 2 bytes per sample * 2 = 4 bytes.
    int16_t s1 = 0;
    int16_t s2 = 10000;

    std::vector<char> data(4);
    std::memcpy(data.data(), &s1, 2);
    std::memcpy(data.data() + 2, &s2, 2);

    auto h = WAVHeader::create(sampleRate, channels, bits, 4);
    auto ss = createStream(h, data);

    WAV wav = WAV::decode(ss);

    ASSERT_TRUE(wav.header.isValid());
    ASSERT_TRUE(wav.rawData.has_value());
    ASSERT_EQ(wav.rawData->size(), 4);
    ASSERT_TRUE(wav.floatData.has_value());
    ASSERT_EQ(wav.floatData->size(), 2);

    // 16-bit conversion uses 1 << 15 = 32768 as coefficient
    EXPECT_FLOAT_EQ(wav.floatData->at(0), 0.0f);
    EXPECT_NEAR(wav.floatData->at(1), 10000.0f / 32768.0f, 1e-5);
}

TEST_F(WAVTest, DecodeValidWAV8Bit) {
    // 8-bit PCM: 1 channel.
    // 8-bit is unsigned 0..255, center at 128
    // monoSample += (static_cast<float>(curSample) - COEFFICIENT) / COEFFICIENT;
    // COEFFICIENT = 1 << (8 - 1) = 128.
    // So (sample - 128) / 128.

    uint16_t bits = 8;
    uint32_t sampleRate = 16000;
    uint16_t channels = 1;

    uint8_t s1 = 128;  // Should be 0.0
    uint8_t s2 = 255;  // Should be (255-128)/128 = 127/128 approx 0.99

    std::vector<char> data(2);
    std::memcpy(data.data(), &s1, 1);
    std::memcpy(data.data() + 1, &s2, 1);

    auto h = WAVHeader::create(sampleRate, channels, bits, 2);
    auto ss = createStream(h, data);

    WAV wav = WAV::decode(ss);

    ASSERT_TRUE(wav.floatData.has_value());
    EXPECT_FLOAT_EQ(wav.floatData->at(0), 0.0f);
    EXPECT_NEAR(wav.floatData->at(1), 127.0f / 128.0f, 1e-5);
}

TEST_F(WAVTest, DecodeThrowsOnShortStream) {
    auto h = createValidHeader(16000, 1, 16, 100);
    std::vector<char> data(50);  // Only 50 bytes, expected 100
    auto ss = createStream(h, data);

    EXPECT_THROW({ WAV::decode(ss); }, std::runtime_error);
}

TEST_F(WAVTest, DecodeHandlesInvalidHeader) {
    auto h = createValidHeader();
    h.numChannels = 0;  // Invalid

    std::stringstream ss;
    ss.write(reinterpret_cast<const char*>(&h), sizeof(WAVHeader));

    WAV wav = WAV::decode(ss);
    EXPECT_FALSE(wav.header.isValid());
    EXPECT_FALSE(wav.rawData.has_value());
}
