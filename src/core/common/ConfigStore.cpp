#include "thoth/ConfigStore.h"

#include <spdlog/spdlog.h>

#include <chrono>
#include <cstdlib>
#include <format>
#include <fstream>

#include "thoth/ConfigKey.h"

namespace fs = std::filesystem;

namespace {
const std::string& launchTimestamp() {
    static const std::string ts = std::format("{:%Y%m%d%H%M%S}", std::chrono::system_clock::now());
    return ts;
}
}  // namespace

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
#ifdef _WIN32
    return fs::temp_directory_path() / "Thoth" / launchTimestamp();
#else
    return fs::path("/tmp") / "Thoth" / launchTimestamp();
#endif
}

std::filesystem::path ConfigStore::getConfigDir() const { return m_configPath.parent_path(); }
std::filesystem::path ConfigStore::getConfiguredPathNoLock(
    const std::string& key, const std::filesystem::path& fallback) const {
    if (!m_config.contains(key)) return fallback;

    try {
        const std::string configuredPath = m_config[key].get<std::string>();
        if (!configuredPath.empty()) {
            return fs::path(configuredPath);
        }
    } catch (const nlohmann::json::exception& e) {
        spdlog::error("Failed to parse configured path [{}]: {}, rolling back to {}", key, e.what(),
                      fallback.string());
    }

    return fallback;
}

std::filesystem::path ConfigStore::getCacheDir() const {
    std::lock_guard<std::mutex> lock(m_configMutex);
    return getConfiguredPathNoLock(thoth::config::KEY_CACHE_DIR, getTempDir() / "cache");
}

std::filesystem::path ConfigStore::getLogDir() const {
    std::lock_guard<std::mutex> lock(m_configMutex);
    return getConfiguredPathNoLock(thoth::config::KEY_LOG_DIR, getTempDir() / "log");
}
std::filesystem::path ConfigStore::getConfigFilePath() const { return m_configPath; }

thoth::GoogleTTSConfig ConfigStore::getGoogleTTSConfig() const {
    std::lock_guard<std::mutex> lock(m_configMutex);
    return thoth::GoogleTTSConfig{
        .languageCode =
            m_config.value(thoth::config::KEY_TTS_LANG, thoth::config::DEFAULT_TTS_LANG),
        .voiceName = m_config.value(thoth::config::KEY_TTS_VOICE, thoth::config::DEFAULT_TTS_VOICE),
        .audioEncoding = m_config.value(thoth::config::KEY_TTS_AUDIO_ENCODING,
                                        thoth::config::DEFAULT_TTS_AUDIO_ENCODING),
    };
}

thoth::LogConfig ConfigStore::getLogConfig() const {
    std::lock_guard<std::mutex> lock(m_configMutex);
    const auto logDirPath =
        getConfiguredPathNoLock(thoth::config::KEY_LOG_DIR, getTempDir() / "log");

    return thoth::LogConfig{
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

thoth::AudioRecorderConfig ConfigStore::getAudioRecorderConfig() const {
    std::lock_guard<std::mutex> lock(m_configMutex);
    return thoth::AudioRecorderConfig{
        .sampleRate = m_config.value<uint32_t>(thoth::config::KEY_AUDIO_RECORDER_SAMPLE_RATE,
                                               thoth::config::DEFAULT_AUDIO_RECORDER_SAMPLE_RATE),
        .channels = m_config.value<uint16_t>(thoth::config::KEY_AUDIO_RECORDER_CHANNELS,
                                             thoth::config::DEFAULT_AUDIO_RECORDER_CHANNELS),
        .sampleFormatBits =
            m_config.value<uint16_t>(thoth::config::KEY_AUDIO_RECORDER_SAMPLE_FORMAT,
                                     thoth::config::DEFAULT_AUDIO_RECORDER_SAMPLE_FORMAT),
        .rmsStep = m_config.value<uint16_t>(thoth::config::KEY_AUDIO_RECORDER_RMS_STEP,
                                            thoth::config::DEFAULT_AUDIO_RECORDER_RMS_STEP),
        .bufferSize = m_config.value<uint32_t>(thoth::config::KEY_AUDIO_RECORDER_BUFFER_SIZE,
                                               thoth::config::DEFAULT_AUDIO_RECORDER_BUFFER_SIZE),
    };
}

thoth::WhisperConfig ConfigStore::getWhisperConfig() const {
    std::lock_guard<std::mutex> lock(m_configMutex);
    return thoth::WhisperConfig{
        .modelPath = m_config.value<std::string>(thoth::config::KEY_WHISPER_MODEL_PATH,
                                                 thoth::config::DEFAULT_WHISPER_MODEL_PATH),
        .language = m_config.value<std::string>(thoth::config::KEY_WHISPER_MODEL_LANGUAGE,
                                                thoth::config::DEFAULT_WHISPER_MODEL_LANGUAGE),
    };
}

std::string ConfigStore::getTTSEngineName() const {
    std::lock_guard<std::mutex> lock(m_configMutex);
    return m_config.value<std::string>(thoth::config::KEY_TTS_ENGINE,
                                       thoth::config::DEFAULT_TTS_ENGINE);
}

std::filesystem::path ConfigStore::getPiperModelPath() const {
    std::lock_guard<std::mutex> lock(m_configMutex);
    return m_config.value<std::string>(thoth::config::KEY_TTS_PIPER_MODEL_PATH,
                                       thoth::config::DEFAULT_TTS_PIPER_MODEL_PATH);
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
