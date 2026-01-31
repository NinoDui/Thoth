#pragma once

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "google/cloud/texttospeech/v1/text_to_speech_client.h"

// TODO: extend the tts service scope

class GCPRuntimeConfig {
   public:
    GCPRuntimeConfig() = default;
    ~GCPRuntimeConfig() = default;

    std::string get(const std::string& key) const;
    void set(const std::string& key, const std::string& value);
    bool isEmpty() const;
    void loadFromFile(const std::string& filePath);

    std::string& operator[](const std::string& key);
    const std::string& operator[](const std::string& key) const;

    friend std::ostream& operator<<(std::ostream& os, const GCPRuntimeConfig& config);

   private:
    std::unordered_map<std::string, std::string> m_config;
};

class GCPTextToSpeechClient {
   public:
    GCPTextToSpeechClient();
    explicit GCPTextToSpeechClient(const std::shared_ptr<GCPRuntimeConfig>& config);
    explicit GCPTextToSpeechClient(
        const google::cloud::texttospeech_v1::TextToSpeechClient& client);
    explicit GCPTextToSpeechClient(const google::cloud::texttospeech_v1::TextToSpeechClient& client,
                                   const std::shared_ptr<GCPRuntimeConfig>& config);

    ~GCPTextToSpeechClient() = default;

    GCPRuntimeConfig& getConfig();
    const GCPRuntimeConfig& getConfig() const;

    std::vector<uint8_t> execute(const std::string& text);

   private:
    google::cloud::texttospeech_v1::TextToSpeechClient m_client;
    std::shared_ptr<GCPRuntimeConfig> m_config;

    void initializeConfig();

    const std::string LANGUAGE_CODE = "en-US";
    const std::string VOICE_NAME = "en-US-Wavenet-1";
    const std::string AUDIO_ENCODING = "MP3";
};
