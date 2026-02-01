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

GCPTextToSpeechClient::GCPTextToSpeechClient()
    : m_client(google::cloud::texttospeech_v1::MakeTextToSpeechConnection()) {}

GCPTextToSpeechClient::GCPTextToSpeechClient(
    const google::cloud::texttospeech_v1::TextToSpeechClient& client)
    : m_client(client) {}

std::vector<uint8_t> GCPTextToSpeechClient::execute(const std::string& text) {
    namespace tts = google::cloud::texttospeech::v1;
    auto ttsConfig = ConfigStore::instance().getGoogleTTSConfig();

    tts::SynthesisInput input;
    input.set_text(text);
    tts::VoiceSelectionParams voiceParam;
    voiceParam.set_language_code(ttsConfig.languageCode);
    voiceParam.set_name(ttsConfig.voiceName);
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
    }(ttsConfig.audioEncoding));

    LOG_TRACE("Synthesizing speech for text: {}", text);
    try {
        auto response = m_client.SynthesizeSpeech(input, voiceParam, audioConfig);
        if (!response) {
            // A HUGE LESSON: google::cloud::Status is not subclass of Exception!!!
            // I should not throw any non-exception anymore (though it's allowed in C++ to throw
            // anything)
            // throw std::move(response).status();
            auto staus = std::move(response).status();
            throw std::runtime_error("GCP call failed [Code " +
                                     std::to_string(static_cast<int>(staus.code())) +
                                     "]: " + staus.message());
        }

        if (!response.ok()) {
            throw std::runtime_error("Failed to synthesize speech: " + response.status().message());
        }

        return std::vector<uint8_t>(response->audio_content().begin(),
                                    response->audio_content().end());
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to synthesize speech: {}", e.what());
        return std::vector<uint8_t>();
    } catch (...) {
        LOG_CRITICAL("Unknown error occurred in synthesizing speech");
        return std::vector<uint8_t>();
    }
}