#pragma once

#include <fmt/ostream.h>

#include <QObject>
#include <filesystem>
#include <mutex>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

#include "thoth/ThothConfig.h"

class ConfigStore : public QObject {
    Q_OBJECT
   public:
    static ConfigStore& instance();

    void init();

    template <typename T>
    void setValue(const std::string& key, const T& value) {
        {
            std::lock_guard<std::mutex> lock(m_configMutex);
            m_config[key] = value;
            save();
        }
        emit configChanged(QString::fromStdString(key));
    }

    template <typename T>
    std::optional<T> getValue(const std::string& key) const {
        std::lock_guard<std::mutex> lock(m_configMutex);
        if (m_config.contains(key)) {
            return m_config[key].get<T>();
        }
        return std::nullopt;
    }

    std::filesystem::path getTempDir() const;
    std::filesystem::path getCacheDir() const;
    std::filesystem::path getLogDir() const;
    std::filesystem::path getConfigFilePath() const;

    friend std::ostream& operator<<(std::ostream& os, const ConfigStore& config);
    void dump() const;

    // Specific object for mutiple possible scenarios
    struct GoogleTTSConfig {
        std::string languageCode;
        std::string voiceName;
        std::string audioEncoding;
    };
    GoogleTTSConfig getGoogleTTSConfig() const;

    struct LogConfig {
        std::string level;
        std::string pattern;
        bool toConsole;
        bool toFile;
        std::filesystem::path logDir;
    };
    LogConfig getLogConfig() const;

    thoth::AudioRecorderConfig getAudioRecorderConfig() const;
    thoth::WhisperConfig getWhisperConfig() const;

    std::string getTTSEngineName() const;
    std::filesystem::path getPiperModelPath() const;

   signals:
    void configChanged(const QString& key);

   private:
    ConfigStore() = default;

    void load();
    void save();

    std::filesystem::path m_configPath;
    std::filesystem::path getConfigDir() const;

    // nlohmann::json read/write is not atomic
    // import explicit mutex to avoid Race Condition
    nlohmann::json m_config = nlohmann::json::object();
    mutable std::mutex m_configMutex;
};

template <>
struct fmt::formatter<ConfigStore> : fmt::ostream_formatter {};
