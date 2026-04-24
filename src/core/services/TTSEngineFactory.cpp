#include "thoth/TTSEngineFactory.h"

#include "internal/GCPTTSEngine.h"
#include "internal/PiperTTSEngine.h"
#include "thoth/ConfigStore.h"
#include "thoth/Logger.h"

namespace thoth {

std::shared_ptr<ITTSEngine> CreateTTSEngine(const std::string& engineName,
                                            const std::string& gcpConfigPath,
                                            const std::string& piperModelPath) {
    if (engineName == "piper") {
        std::filesystem::path modelPath(piperModelPath);
        auto engine = std::make_shared<PiperTTSEngine>(modelPath);
        if (engine->isAvailable()) {
            LOG_INFO("Created Piper TTS engine");
            return engine;
        }
        LOG_WARN("Piper engine unavailable, falling back to GCP");
    }

    auto& config = ConfigStore::instance();
    LOG_INFO("Creating GCP TTS engine");
    return std::make_shared<GCPTTSEngine>(config.getGoogleTTSConfig());
}

}  // namespace thoth
