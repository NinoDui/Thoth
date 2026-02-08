#pragma once

#include <QAudioFormat>
#include <cstdint>

namespace thoth::internal {
inline uint16_t QAudioSampleFormatToBits(QAudioFormat::SampleFormat format) {
    switch (format) {
        case QAudioFormat::SampleFormat::UInt8:
            return 8;
        case QAudioFormat::SampleFormat::Int16:
            return 16;
        case QAudioFormat::SampleFormat::Int32:
            return 32;
        case QAudioFormat::SampleFormat::Float:
            return 64;
        default:
            return 16;
    }
}
}  // namespace thoth::internal