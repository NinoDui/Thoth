#include "internal/GCPTTSEngine.h"

GCPTTSEngine::GCPTTSEngine(const ConfigStore::GoogleTTSConfig& config)
    : m_client(std::make_unique<GCPTextToSpeechClient>(config)), m_config(config) {}

thoth::TTSAudioResult GCPTTSEngine::synthesize(const std::string& text) {
    auto rawBytes = m_client->execute(text);
    return thoth::TTSAudioResult{
        .audioData = rawBytes,
        .format = m_config.audioEncoding,
        .sampleRate = 0,
    };
}

std::string GCPTTSEngine::engineName() const { return "GCP"; }

bool GCPTTSEngine::isAvailable() const { return true; }
