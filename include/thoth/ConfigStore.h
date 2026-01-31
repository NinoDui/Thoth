#pragma once

#include <fmt/ostream.h>

#include <filesystem>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

class ConfigStore {
   public:
    static ConfigStore& instance();

    void init();

    template <typename T>
    void setValue(const std::string& key, const T& value) {
        m_config[key] = value;
        save();
    }

    template <typename T>
    std::optional<T> getValue(const std::string& key) const {
        if (m_config.contains(key)) {
            return m_config[key].get<T>();
        }
        return std::nullopt;
    }

    std::filesystem::path getTempDir() const;
    std::filesystem::path getCacheDir() const;
    std::filesystem::path getLogDir() const;

    friend std::ostream& operator<<(std::ostream& os, const ConfigStore& config);

    void dump() const;

   private:
    ConfigStore() = default;

    void load();
    void save();

    std::filesystem::path getConfigDir() const;

    std::filesystem::path m_configPath;
    nlohmann::json m_config;
};

template <>
struct fmt::formatter<ConfigStore> : fmt::ostream_formatter {};