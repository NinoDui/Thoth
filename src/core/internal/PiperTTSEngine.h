#pragma once

#include <filesystem>
#include <string>

#include "thoth/ITTSEngine.h"

class PiperTTSEngine : public thoth::ITTSEngine {
   public:
    PiperTTSEngine(const std::filesystem::path& modelPath);

    thoth::TTSAudioResult synthesize(const std::string& text) override;
    std::string engineName() const override;
    bool isAvailable() const override;

   private:
    std::filesystem::path m_modelPath;
    bool m_initialized = false;
};
