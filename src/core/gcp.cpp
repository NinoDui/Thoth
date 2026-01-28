#include "gcp.h"

#include <google/cloud/texttospeech/v1/text_to_speech_connection.h>

#include <fstream>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <vector>

std::string GCPRuntimeConfig::get(const std::string& key) const { return m_config.at(key); }

void GCPRuntimeConfig::set(const std::string& key, const std::string& value) {
    m_config[key] = value;
}

void GCPRuntimeConfig::loadFromFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filePath);
    }

    nlohmann::json json;
    file >> json;
    m_config = json.get<std::unordered_map<std::string, std::string>>();
    file.close();
}

std::string& GCPRuntimeConfig::operator[](const std::string& key) { return m_config[key]; }

const std::string& GCPRuntimeConfig::operator[](const std::string& key) const {
    return m_config.at(key);
}

std::ostream& operator<<(std::ostream& os, const GCPRuntimeConfig& config) {
    os << "GCPRuntimeConfig: {" << std::endl;
    for (const auto& [key, value] : config.m_config) {
        os << "  " << key << ": " << value << std::endl;
    }
    os << "}" << std::endl;
    return os;
}

GCPTextToSpeechClient::GCPTextToSpeechClient()
    : m_client(google::cloud::texttospeech_v1::MakeTextToSpeechConnection()) {
    m_config = std::make_shared<GCPRuntimeConfig>();
    m_config->set("languageCode", LANGUAGE_CODE);
    m_config->set("voiceName", VOICE_NAME);
    m_config->set("audioEncoding", AUDIO_ENCODING);
}

GCPTextToSpeechClient::GCPTextToSpeechClient(
    const google::cloud::texttospeech_v1::TextToSpeechClient& client)
    : m_client(client) {
    m_config = std::make_shared<GCPRuntimeConfig>();
    m_config->set("languageCode", LANGUAGE_CODE);
    m_config->set("voiceName", VOICE_NAME);
    m_config->set("audioEncoding", AUDIO_ENCODING);
}

GCPTextToSpeechClient::GCPTextToSpeechClient(
    const google::cloud::texttospeech_v1::TextToSpeechClient& client,
    const std::shared_ptr<GCPRuntimeConfig>& config)
    : m_client(client), m_config(config) {}

std::vector<uint8_t> GCPTextToSpeechClient::execute(const std::string& text) {
    namespace tts = google::cloud::texttospeech::v1;
    tts::SynthesisInput input;
    input.set_text(text);
    tts::VoiceSelectionParams voiceParam;
    voiceParam.set_language_code(m_config->get("languageCode"));
    tts::AudioConfig audioConfig;
    audioConfig.set_audio_encoding(tts::AudioEncoding::MP3);

    auto response = m_client.SynthesizeSpeech(input, voiceParam, audioConfig);
    if (!response) {
        throw std::move(response).status();
    }

    // FOR DEBUG ONLY, AVOID USING PRINTOUT IN PRODUCTION
    std::cout << "Response Status: " << response.status() << std::endl;
    std::cout << "Response Bytes Size: " << response->ByteSizeLong() << std::endl;
    // FOR DEBUG END

    if (!response.ok()) {
        throw std::runtime_error("Failed to synthesize speech: " + response.status().message());
    }

    return std::vector<uint8_t>(response->audio_content().begin(), response->audio_content().end());
}