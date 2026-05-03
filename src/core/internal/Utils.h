#pragma once

#include <QAudioFormat>
#include <cstdint>

namespace thoth::internal {
uint16_t QAudioSampleFormatToBits(QAudioFormat::SampleFormat format);
}  // namespace thoth::internal
