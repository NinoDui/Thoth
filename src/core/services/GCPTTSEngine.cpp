#include "internal/GCPTTSEngine.h"

GCPTTSEngine::GCPTTSEngine(const thoth::GoogleTTSConfig& config)
    : m_client(std::make_unique<GCPTextToSpeechClient>(config)), m_config(config) {}

thoth::TTSAudioResult GCPTTSEngine::synthesize(const std::string& text) {
    auto rawBytes = m_client->execute(text);
    std::string format = m_config.audioEncoding;
    for (auto& c : format) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (format == "linear16") {
        format = "wav";
    }
    return thoth::TTSAudioResult{
        .audioData = rawBytes,
        .format = format,
        .sampleRate = 0,
    };
}

std::string GCPTTSEngine::engineName() const { return "GCP"; }

std::string GCPTTSEngine::cacheIdentity() const {
    return "gcp|" + m_config.languageCode + "|" + m_config.voiceName + "|" + m_config.audioEncoding;
}

bool GCPTTSEngine::isAvailable() const { return true; }
