#pragma once

#ifndef SPDLOG_ACTIVE_LEVEL
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#endif

#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>

#include "thoth/ConfigStore.h"

#define LOG_TRACE(...) SPDLOG_TRACE(__VA_ARGS__)
#define LOG_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)
#define LOG_INFO(...) SPDLOG_INFO(__VA_ARGS__)
#define LOG_WARN(...) SPDLOG_WARN(__VA_ARGS__)
#define LOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
#define LOG_CRITICAL(...) SPDLOG_CRITICAL(__VA_ARGS__)

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
