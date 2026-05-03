#pragma once

#include <memory>

#include "internal/GCP.h"
#include "thoth/ConfigStore.h"
#include "thoth/ITTSEngine.h"

class GCPTTSEngine : public thoth::ITTSEngine {
   public:
    explicit GCPTTSEngine(const thoth::GoogleTTSConfig& config);
    ~GCPTTSEngine() override = default;

    thoth::TTSAudioResult synthesize(const std::string& text) override;
    std::string engineName() const override;
    bool isAvailable() const override;

   private:
    std::unique_ptr<GCPTextToSpeechClient> m_client;
    thoth::GoogleTTSConfig m_config;
};
