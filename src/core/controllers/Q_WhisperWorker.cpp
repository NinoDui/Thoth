#include "internal/Q_WhisperWorker.h"

#include <whisper.h>

#include <algorithm>
#include <filesystem>

#include "internal/InternalEntity.h"
#include "thoth/Logger.h"

Q_WhisperWorker::Q_WhisperWorker(const thoth::WhisperConfig& config, QObject* parent)
    : QObject(parent), m_config(config) {}

Q_WhisperWorker::~Q_WhisperWorker() {
    if (m_ctx) {
        whisper_free(m_ctx);
        m_ctx = nullptr;
    }
}

bool Q_WhisperWorker::ensureContext() {
    if (m_ctx) {
        return true;
    }

    whisper_context_params cparams = whisper_context_default_params();
    m_ctx = whisper_init_from_file_with_params(m_config.modelPath.c_str(), cparams);
    if (!m_ctx) {
        LOG_ERROR("Failed to load Whisper model from: {}", m_config.modelPath);
        return false;
    }
    LOG_INFO("Whisper model loaded from: {}", m_config.modelPath);
    return true;
}

void Q_WhisperWorker::doTranscribe(RecordedSentence* rs) {
    emit busyChanged(true);

    if (!ensureContext()) {
        emit errorOccurred("Failed to load Whisper model. Check model path in Settings.");
        emit busyChanged(false);
        return;
    }

    WAV wav;
    try {
        wav = WAV::decode(rs->localShadowingPath.string());
    } catch (const std::exception& e) {
        emit errorOccurred(QString("WAV decode failed: %1").arg(e.what()));
        emit busyChanged(false);
        return;
    }

    if (!wav.floatData || wav.floatData->empty()) {
        emit errorOccurred("WAV decode produced no audio data.");
        emit busyChanged(false);
        return;
    }

    whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    wparams.language = m_config.language.c_str();
    wparams.print_progress = false;
    wparams.print_timestamps = false;

    int ret = whisper_full(m_ctx, wparams, wav.floatData->data(),
                           static_cast<int>(wav.floatData->size()));
    if (ret != 0) {
        emit errorOccurred("Whisper transcription failed.");
        emit busyChanged(false);
        return;
    }

    std::string result;
    int nSegments = whisper_full_n_segments(m_ctx);
    for (int i = 0; i < nSegments; ++i) {
        result += whisper_full_get_segment_text(m_ctx, i);
    }
    // whisper often prepends a leading space — trim it
    if (!result.empty() && result.front() == ' ') {
        result.erase(0, 1);
    }
    rs->transcribedText = result;

    LOG_DEBUG("Transcribed: \"{}\"", result);
    emit transcriptReady(rs);
    emit busyChanged(false);
}

void Q_WhisperWorker::doTranscribeFile(const QString& audioPath) {
    emit busyChanged(true);

    if (!ensureContext()) {
        emit errorOccurred("Failed to load Whisper model. Check model path in Settings.");
        emit busyChanged(false);
        return;
    }

    std::string pathStr = audioPath.toStdString();
    std::string ext = std::filesystem::path(pathStr).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    std::vector<float> audioData;

    if (ext == ".wav") {
        WAV wav;
        try {
            wav = WAV::decode(pathStr);
        } catch (const std::exception& e) {
            emit errorOccurred(QString("WAV decode failed: %1").arg(e.what()));
            emit busyChanged(false);
            return;
        }
        if (!wav.floatData || wav.floatData->empty()) {
            emit errorOccurred("WAV decode produced no audio data.");
            emit busyChanged(false);
            return;
        }
        if (wav.header.sampleRate != 16000) {
            wav = WAV::resample(wav, 16000);
            if (!wav.floatData || wav.floatData->empty()) {
                emit errorOccurred("Audio resampling to 16kHz failed.");
                emit busyChanged(false);
                return;
            }
        }
        audioData = std::move(*wav.floatData);
    } else {
        emit errorOccurred(
            QString("Unsupported audio format '%1'. Only WAV is supported for transcription.")
                .arg(QString::fromStdString(ext)));
        emit busyChanged(false);
        return;
    }

    whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    wparams.language = m_config.language.c_str();
    wparams.print_progress = false;
    wparams.print_timestamps = true;

    int ret = whisper_full(m_ctx, wparams, audioData.data(), static_cast<int>(audioData.size()));
    if (ret != 0) {
        emit errorOccurred("Whisper transcription of audio file failed.");
        emit busyChanged(false);
        return;
    }

    std::vector<TranscriptSegment> segments;
    int nSegments = whisper_full_n_segments(m_ctx);
    for (int i = 0; i < nSegments; ++i) {
        std::string text = whisper_full_get_segment_text(m_ctx, i);
        if (!text.empty() && text.front() == ' ') {
            text.erase(0, 1);
        }
        if (text.empty()) {
            continue;
        }

        // whisper timestamps are in centiseconds (hundredths of a second)
        double startMs = static_cast<double>(whisper_full_get_segment_t0(m_ctx, i)) * 10.0;
        double endMs = static_cast<double>(whisper_full_get_segment_t1(m_ctx, i)) * 10.0;
        segments.push_back({std::move(text), startMs, endMs});
    }

    LOG_DEBUG("Transcribed {} segments from file: {}", segments.size(), pathStr);
    emit transcriptSegmentsReady(std::move(segments));
    emit busyChanged(false);
}

void Q_WhisperWorker::reloadModel(const thoth::WhisperConfig& config) {
    m_config = config;
    if (m_ctx) {
        whisper_free(m_ctx);
        m_ctx = nullptr;
        LOG_INFO("Whisper model unloaded, will reload with new config on next transcribe");
    }
}
