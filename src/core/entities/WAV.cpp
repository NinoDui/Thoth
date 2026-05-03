#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>

#include "Thoth/Logger.h"
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

WAVHeader WAVHeader::read(const std::string& filename) {
    std::ifstream inputFile(filename, std::ios::binary | std::ios::in);
    if (!inputFile || !inputFile.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    auto header = WAVHeader::read(inputFile);
    inputFile.close();
    return header;
}

WAVHeader WAVHeader::read(std::istream& inputFile) {
    constexpr size_t targetSize = sizeof(WAVHeader);
    char buffer[targetSize];
    inputFile.read(buffer, static_cast<std::streamsize>(targetSize));
    const size_t bytesRead = static_cast<size_t>(inputFile.gcount());
    if (bytesRead != targetSize) {
        throw std::runtime_error("WAV Header reading failed, expected " +
                                 std::to_string(targetSize) + " bytes, got " +
                                 std::to_string(bytesRead));
    }

    WAVHeader h;
    std::memcpy(&h, buffer, targetSize);
    return h;
}

bool WAVHeader::isValid() const {
    if (dataSize % blockAlign != 0) {
        // Case 1, the overall datasize in bytes is not divisible by the block align (per frame),
        // where blockalign = sample per channel * numChannels * bitsperChannel / 8
        LOG_ERROR("WAV Header is invalid, dataSize {} is not divisible by blockAlign {}", dataSize,
                  blockAlign);
        return false;
    }
    if (fmtType != 1) {
        // Case 2, the format type is not PCM, which is not supported
        LOG_ERROR("WAV Header is invalid, fmtType {} is not supported", fmtType);
        return false;
    }
    if (numChannels < 1) {
        // Case 3
        LOG_ERROR("WAV Header is invalid, numChannels {} is less than 1", numChannels);
        return false;
    }
    return true;
}

std::ostream& operator<<(std::ostream& os, const WAVHeader& header) {
    os << "WAVHeader(riff=" << header.riff << ", overallSize=" << header.overallSize
       << ", wave=" << header.wave << ", fmt=" << header.fmt << ", fmtLength=" << header.fmtLength
       << ", fmtType=" << header.fmtType << ", numChannels=" << header.numChannels
       << ", sampleRate=" << header.sampleRate << ", byteRate=" << header.byteRate
       << ", blockAlign=" << header.blockAlign << ", bitsPerSample=" << header.bitsPerSample
       << ", data=" << header.data << ", dataSize=" << header.dataSize << ")";
    return os;
}

void _wavToFloat(WAV& wav) {
    // sample per channel
    //  -> one amplitude value for one channel at one time step
    wav.floatData = std::vector<float>();
    size_t i = 0;  // iterate through the raw data in bytes
    while (i < wav.rawData->size()) {
        float monoSample = 0.0f;
        size_t COEFFICIENT = 1 << (wav.header.bitsPerSample - 1);
        // for each channel, get the sample with presettings
        for (size_t ch = 0; ch < wav.header.numChannels; ch++) {
            switch (wav.header.bitsPerSample) {
                case 8: {
                    uint8_t curSample;
                    std::memcpy(&curSample, wav.rawData->data() + i, sizeof(uint8_t));
                    monoSample += (static_cast<float>(curSample) - COEFFICIENT) / COEFFICIENT;
                    break;
                }
                case 16: {
                    int16_t curSample;
                    std::memcpy(&curSample, wav.rawData->data() + i, sizeof(int16_t));
                    monoSample += static_cast<float>(curSample) / COEFFICIENT;
                    break;
                }
                case 32: {
                    int32_t curSample;
                    std::memcpy(&curSample, wav.rawData->data() + i, sizeof(int32_t));
                    monoSample += static_cast<float>(curSample) / COEFFICIENT;
                    break;
                }
                default:
                    throw std::runtime_error("Unsupported bits per sample: " +
                                             std::to_string(wav.header.bitsPerSample));
            }
            i += wav.header.bitsPerSample / 8;
        }

        monoSample /= wav.header.numChannels;
        wav.floatData->push_back(monoSample);
    }
}

WAV WAV::decode(std::istream& inputStream) {
    WAV wav;
    wav.header = WAVHeader::read(inputStream);
    if (!wav.header.isValid()) {
        wav.rawData = std::nullopt;
        wav.floatData = std::nullopt;
        return wav;
    }

    wav.rawData = std::vector<char>(wav.header.dataSize);
    inputStream.read(wav.rawData->data(), static_cast<std::streamsize>(wav.header.dataSize));
    const size_t bytesRead = static_cast<size_t>(inputStream.gcount());
    if (bytesRead != wav.header.dataSize) {
        throw std::runtime_error("WAV Data reading failed, expected " +
                                 std::to_string(wav.header.dataSize) + " bytes, got " +
                                 std::to_string(bytesRead));
    }
    LOG_DEBUG("WAV Data read: {} bytes, expected {}", bytesRead, wav.header.dataSize);

    try {
        _wavToFloat(wav);
    } catch (const std::runtime_error& e) {
        LOG_ERROR("Failed to convert WAV to float: {}", e.what());
        wav.floatData = std::nullopt;
    } catch (...) {
        LOG_CRITICAL("Unknown error occurred in converting WAV data to float");
        wav.floatData = std::nullopt;
    }
    return wav;
}

WAV WAV::decode(const std::string& filename) {
    std::ifstream inputFile(filename, std::ios::binary | std::ios::in);
    if (!inputFile || !inputFile.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }
    return decode(inputFile);
}

WAV WAV::resample(const WAV& wav, uint32_t newSampleRate) {
    if (wav.header.sampleRate == newSampleRate) return wav;
    if (!wav.floatData || wav.floatData->empty()) return WAV();

    const double ratio = static_cast<double>(wav.header.sampleRate) / newSampleRate;
    const size_t srcSize = wav.floatData->size();
    const size_t dstSize = static_cast<size_t>(std::ceil(static_cast<double>(srcSize) / ratio));

    WAV result;
    result.floatData = std::vector<float>(dstSize);
    for (size_t i = 0; i < dstSize; ++i) {
        double srcPos = static_cast<double>(i) * ratio;
        auto srcIdx = static_cast<size_t>(srcPos);
        double frac = srcPos - static_cast<double>(srcIdx);
        float s0 = (*wav.floatData)[srcIdx];
        float s1 = (srcIdx + 1 < srcSize) ? (*wav.floatData)[srcIdx + 1] : s0;
        (*result.floatData)[i] = static_cast<float>(s0 * (1.0 - frac) + s1 * frac);
    }

    result.header = wav.header;
    result.header.sampleRate = newSampleRate;
    result.header.byteRate = newSampleRate * wav.header.blockAlign;
    LOG_INFO("Resampled audio from {}Hz to {}Hz ({} -> {} samples)", wav.header.sampleRate,
             newSampleRate, srcSize, dstSize);
    return result;
}

std::ostream& operator<<(std::ostream& os, const WAV& wav) {
    os << "WAV(header=" << wav.header
       << ", rawDataSize=" << (wav.rawData.has_value() ? wav.rawData->size() : 0)
       << ", floatDataSize=" << (wav.floatData.has_value() ? wav.floatData->size() : 0) << ")";
    return os;
}
