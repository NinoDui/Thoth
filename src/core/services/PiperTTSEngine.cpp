#include "internal/PiperTTSEngine.h"

#include "thoth/Logger.h"

PiperTTSEngine::PiperTTSEngine(const std::filesystem::path& modelPath) : m_modelPath(modelPath) {
    if (!std::filesystem::exists(modelPath)) {
        LOG_WARN("Piper model not found at: {}", modelPath.string());
        return;
    }

    m_initialized = true;
    LOG_INFO("Piper TTS engine initialized with model: {}", modelPath.string());
}

thoth::TTSAudioResult PiperTTSEngine::synthesize(const std::string& text) {
    if (!m_initialized) {
        LOG_ERROR("Piper engine not initialized");
        return {};
    }

    LOG_WARN("Piper synthesis not yet implemented. Install piper and link libpiper_phonemize.");
    return {};
}

std::string PiperTTSEngine::engineName() const { return "Piper"; }

std::string PiperTTSEngine::cacheIdentity() const { return "piper|" + m_modelPath.string(); }

bool PiperTTSEngine::isAvailable() const { return m_initialized; }
