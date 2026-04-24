#pragma once

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "google/cloud/texttospeech/v1/text_to_speech_client.h"
#include "thoth/ConfigStore.h"

class GCPTextToSpeechClient {
   public:
    explicit GCPTextToSpeechClient(const ConfigStore::GoogleTTSConfig& config);
    explicit GCPTextToSpeechClient(const google::cloud::texttospeech_v1::TextToSpeechClient& client,
                                   const ConfigStore::GoogleTTSConfig& config);

    ~GCPTextToSpeechClient() = default;

    std::vector<uint8_t> execute(const std::string& text);

   private:
    google::cloud::texttospeech_v1::TextToSpeechClient m_client;
    ConfigStore::GoogleTTSConfig m_config;
};
