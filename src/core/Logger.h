#pragma once

#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>

#define LOG_INFO(...) spdlog::info(__VA_ARGS__)
#define LOG_ERROR(...) spdlog::error(__VA_ARGS__)
#define LOG_WARNING(...) spdlog::warn(__VA_ARGS__)
#define LOG_CRITICAL(...) spdlog::critical(__VA_ARGS__)
#define LOG_DEBUG(...) spdlog::debug(__VA_ARGS__)
#define LOG_TRACE(...) spdlog::trace(__VA_ARGS__)
#define LOG_FATAL(...) spdlog::fatal(__VA_ARGS__)

class LogManager {
   public:
    struct Guard {
        Guard(const std::string& appName) { LogManager::init(appName); }
        ~Guard() { LogManager::shutdown(); }

        // Copy operation is forbidden
        Guard(const Guard&) = delete;
        Guard& operator=(const Guard&) = delete;
    };

   private:
    static void init(const std::string& appName);
    static void shutdown();

    friend struct Guard;

    static constexpr int QUEUE_LENGTH = 8192;
    static constexpr int FLUSH_INTERVAL_SECONDS = 5;
};