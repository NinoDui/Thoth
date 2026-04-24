#pragma once

#include <memory>
#include <string>

#include "thoth/ITTSEngine.h"

namespace thoth {

std::shared_ptr<ITTSEngine> CreateTTSEngine(const std::string& engineName,
                                            const std::string& gcpConfigPath,
                                            const std::string& piperModelPath);

}  // namespace thoth
