#pragma once

#include <filesystem>
#include <string>

#include "thoth/ITTSEngine.h"

class PiperTTSEngine : public thoth::ITTSEngine {
   public:
    explicit PiperTTSEngine(const std::filesystem::path& modelPath);

    thoth::TTSAudioResult synthesize(const std::string& text) override;
    std::string engineName() const override;
    std::string cacheIdentity() const override;
    bool isAvailable() const override;

   private:
    std::filesystem::path m_modelPath;
    bool m_initialized = false;
};
