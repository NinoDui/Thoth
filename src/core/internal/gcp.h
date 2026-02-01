#pragma once

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "google/cloud/texttospeech/v1/text_to_speech_client.h"

// TODO: extend the tts service scope

class GCPTextToSpeechClient {
   public:
    GCPTextToSpeechClient();
    explicit GCPTextToSpeechClient(
        const google::cloud::texttospeech_v1::TextToSpeechClient& client);

    ~GCPTextToSpeechClient() = default;

    std::vector<uint8_t> execute(const std::string& text);

   private:
    google::cloud::texttospeech_v1::TextToSpeechClient m_client;
};
