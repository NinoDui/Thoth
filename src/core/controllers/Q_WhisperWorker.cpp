#include "internal/Q_WhisperWorker.h"

#include <whisper.h>

#include "internal/InternalEntity.h"
#include "thoth/ConfigKey.h"
#include "thoth/ConfigStore.h"
#include "thoth/Logger.h"

Q_WhisperWorker::Q_WhisperWorker(QObject* parent) : QObject(parent) {}

Q_WhisperWorker::~Q_WhisperWorker() {
    if (m_ctx) {
        whisper_free(m_ctx);
        m_ctx = nullptr;
    }
}

bool Q_WhisperWorker::ensureContext() {
    if (m_ctx) return true;

    auto modelPathOpt =
        ConfigStore::instance().getValue<std::string>(thoth::config::KEY_WHISPER_MODEL_PATH);
    std::string modelPath =
        modelPathOpt.value_or(std::string(thoth::config::DEFAULT_WHISPER_MODEL_PATH));

    whisper_context_params cparams = whisper_context_default_params();
    m_ctx = whisper_init_from_file_with_params(modelPath.c_str(), cparams);
    if (!m_ctx) {
        LOG_ERROR("Failed to load Whisper model from: {}", modelPath);
        return false;
    }
    LOG_INFO("Whisper model loaded from: {}", modelPath);
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
    auto langOpt =
        ConfigStore::instance().getValue<std::string>(thoth::config::KEY_WHISPER_MODEL_LANGUAGE);
    std::string lang = langOpt.value_or(std::string(thoth::config::DEFAULT_WHISPER_MODEL_LANGUAGE));
    wparams.language = lang.c_str();
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
    if (!result.empty() && result.front() == ' ') result.erase(0, 1);
    rs->transcribedText = result;

    LOG_DEBUG("Transcribed: \"{}\"", result);
    emit transcriptReady(rs);
    emit busyChanged(false);
}
