#include "thoth/ConfigStore.h"

#include <spdlog/spdlog.h>

#include <cstdlib>
#include <fstream>

#include "thoth/ConfigKey.h"

namespace fs = std::filesystem;

ConfigStore& ConfigStore::instance() {
    static ConfigStore instance;
    return instance;
}

void ConfigStore::init() {
    fs::path homeDir;
#ifdef _WIN32
    char* env_buf = nullptr;
    size_t len = 0;
    if (_dupenv_s(&env_buf, &len, "USERPROFILE") == 0 && env_buf != nullptr) {
        homeDir = std::string(env_buf);
        free(env_buf);
    } else {
        spdlog::error("Failed to get USERPROFILE environment variable");
        homeDir = fs::path();
    }
#else
    if (const char* env_p = std::getenv("HOME")) homeDir = env_p;
#endif
    auto configDir = homeDir / ".config" / "Thoth";
    if (!fs::exists(configDir)) {
        fs::create_directories(configDir);
    }
    m_configPath = configDir / "config.json";
    spdlog::info("Configuration Store Path: {}", m_configPath.string());
    load();
}

void ConfigStore::load() {
    if (fs::exists(m_configPath)) {
        std::ifstream file(m_configPath);
        if (file.is_open()) {
            file >> m_config;
            spdlog::info("Loaded {} configuration entries", m_config.size());
        } else {
            spdlog::error("Failed to open configuration file: {}", m_configPath.string());
        }
        file.close();
    }
}

void ConfigStore::save() {
    std::ofstream file(m_configPath);
    if (file.is_open()) {
        file << m_config.dump(4);
        spdlog::debug("Saved {} configuration entries", m_config.size());
    } else {
        spdlog::error("Failed to open configuration file when saving: {}", m_configPath.string());
    }
    file.close();
}

std::filesystem::path ConfigStore::getTempDir() const {
    return fs::temp_directory_path() / "Thoth";
}

std::filesystem::path ConfigStore::getConfigDir() const { return m_configPath.parent_path(); }
std::filesystem::path ConfigStore::getCacheDir() const { return getTempDir() / "cache"; }
std::filesystem::path ConfigStore::getLogDir() const { return getTempDir() / "log"; }

ConfigStore::GoogleTTSConfig ConfigStore::getGoogleTTSConfig() const {
    std::lock_guard<std::mutex> lock(m_configMutex);
    return GoogleTTSConfig{
        .languageCode =
            m_config.value(thoth::config::KEY_TTS_LANG, thoth::config::DEFAULT_TTS_LANG),
        .voiceName = m_config.value(thoth::config::KEY_TTS_VOICE, thoth::config::DEFAULT_TTS_VOICE),
        .audioEncoding = m_config.value(thoth::config::KEY_TTS_AUDIO_ENCODING,
                                        thoth::config::DEFAULT_TTS_AUDIO_ENCODING),
    };
}

ConfigStore::LogConfig ConfigStore::getLogConfig() const {
    std::lock_guard<std::mutex> lock(m_configMutex);

    // specify the default or user-defined log directory
    std::filesystem::path logDirPath = getLogDir();
    if (m_config.contains(thoth::config::KEY_LOG_DIR)) {
        try {
            std::string logDirStr = m_config[thoth::config::KEY_LOG_DIR].get<std::string>();
            if (!logDirStr.empty()) {
                logDirPath = fs::path(logDirStr);
            }
        } catch (const nlohmann::json::exception& e) {
            spdlog::error(
                "Failed to parse log directory: {}, rolling back to default log directory {}",
                e.what(), logDirPath.string());
        }
    }
    return LogConfig{
        .level = m_config.value(thoth::config::KEY_LOG_LEVEL, thoth::config::DEFAULT_LOG_LEVEL),
        .pattern =
            m_config.value(thoth::config::KEY_LOG_PATTERN, thoth::config::DEFAULT_LOG_PATTERN),
        .toConsole = m_config.value(thoth::config::KEY_LOG_TO_CONSOLE,
                                    thoth::config::DEFAULT_LOG_TO_CONSOLE),
        .toFile =
            m_config.value(thoth::config::KEY_LOG_TO_FILE, thoth::config::DEFAULT_LOG_TO_FILE),
        .logDir = logDirPath,
    };
}

std::ostream& operator<<(std::ostream& os, const ConfigStore& config) {
    os << "ConfigStore: {" << std::endl;
    os << "[temp] " << config.getTempDir().string() << std::endl;
    os << "[cache] " << config.getCacheDir().string() << std::endl;
    os << "[config] {" << std::endl;
    if (config.m_config.empty()) {
        os << "  (empty)\n}" << std::endl;
        return os;
    }
    for (const auto& [key, value] : config.m_config.items()) {
        os << "  " << key << ": " << value.get<std::string>() << std::endl;
    }
    os << "}" << std::endl;
    return os;
}

void ConfigStore::dump() const { spdlog::info("{}", *this); }