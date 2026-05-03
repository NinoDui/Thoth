#include "internal/gcp.h"

#include <google/cloud/texttospeech/v1/text_to_speech_connection.h>

#include <fstream>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <vector>

#include "thoth/ConfigStore.h"
#include "thoth/Logger.h"

GCPTextToSpeechClient::GCPTextToSpeechClient(const thoth::GoogleTTSConfig& config)
    : m_client(google::cloud::texttospeech_v1::MakeTextToSpeechConnection()), m_config(config) {}

GCPTextToSpeechClient::GCPTextToSpeechClient(
    const google::cloud::texttospeech_v1::TextToSpeechClient& client,
    const thoth::GoogleTTSConfig& config)
    : m_client(client), m_config(config) {}

std::vector<uint8_t> GCPTextToSpeechClient::execute(const std::string& text) {
    namespace tts = google::cloud::texttospeech::v1;

    tts::SynthesisInput input;
    input.set_text(text);
    tts::VoiceSelectionParams voiceParam;
    voiceParam.set_language_code(m_config.languageCode);
    voiceParam.set_name(m_config.voiceName);
    tts::AudioConfig audioConfig;
    audioConfig.set_audio_encoding([](const std::string& enc) -> tts::AudioEncoding {
        if (enc == "MP3" || enc == "mp3") {
            return tts::AudioEncoding::MP3;
        } else if (enc == "OGG_OPUS" || enc == "ogg_opus") {
            return tts::AudioEncoding::OGG_OPUS;
        } else if (enc == "WAV" || enc == "wav") {
            return tts::AudioEncoding::LINEAR16;
        } else if (enc == "MULAW" || enc == "mulaw") {
            return tts::AudioEncoding::MULAW;
        } else {
            return tts::AudioEncoding::MP3;
        }
    }(m_config.audioEncoding));

    LOG_TRACE("Synthesizing speech for text: {}", text);
    auto response = m_client.SynthesizeSpeech(input, voiceParam, audioConfig);
    if (!response) {
        // google::cloud::Status is not a subclass of std::exception.
        // Wrap it so callers can catch uniformly.
        auto status = std::move(response).status();
        throw std::runtime_error("GCP call failed [Code " +
                                 std::to_string(static_cast<int>(status.code())) +
                                 "]: " + status.message());
    }

    return std::vector<uint8_t>(response->audio_content().begin(), response->audio_content().end());
}

std::vector<thoth::GoogleVoiceInfo> GCPTextToSpeechClient::listVoices(
    const std::string& languageCode) {
    namespace tts = google::cloud::texttospeech::v1;

    auto response = m_client.ListVoices(languageCode);
    if (!response) {
        auto status = std::move(response).status();
        throw std::runtime_error("GCP ListVoices failed [Code " +
                                 std::to_string(static_cast<int>(status.code())) +
                                 "]: " + status.message());
    }

    std::vector<thoth::GoogleVoiceInfo> results;
    for (const auto& v : response->voices()) {
        thoth::GoogleVoiceInfo info;
        info.name = v.name();
        info.ssmlGender = tts::SsmlVoiceGender_Name(v.ssml_gender());
        info.naturalSampleRateHertz = v.natural_sample_rate_hertz();
        for (const auto& lc : v.language_codes()) {
            info.languageCodes.push_back(lc);
        }
        results.push_back(std::move(info));
    }

    LOG_INFO("GCP ListVoices returned {} voices for language '{}'", results.size(), languageCode);
    return results;
}
