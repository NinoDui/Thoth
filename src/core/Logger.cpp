#include "thoth/Logger.h"

#include <spdlog/async.h>
#include <spdlog/common.h>
#include <spdlog/sinks/hourly_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <QMessageLogContext>
#include <QString>
#include <iostream>

void qtMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
    QByteArray localMsg = msg.toLocal8Bit();
    switch (type) {
        case QtDebugMsg:
            spdlog::debug("Qt: {}", localMsg.constData());
            break;
        case QtInfoMsg:
            spdlog::info("Qt: {}", localMsg.constData());
            break;
        case QtWarningMsg:
            spdlog::warn("Qt: {}", localMsg.constData());
            break;
        case QtCriticalMsg:
            spdlog::critical("Qt: {}", localMsg.constData());
            break;
        case QtFatalMsg:
            spdlog::critical("Qt: {}", localMsg.constData());
            break;
        default:
            spdlog::error("Qt: {}", localMsg.constData());
            break;
    }
}

void LogManager::init(const std::string& appName) {
    static const std::unordered_map<std::string, spdlog::level::level_enum> LEVEL_MAP = {
        {"trace", spdlog::level::trace}, {"debug", spdlog::level::debug},
        {"info", spdlog::level::info},   {"warn", spdlog::level::warn},
        {"error", spdlog::level::err},   {"critical", spdlog::level::critical},
    };

    auto config = ConfigStore::instance().getLogConfig();
    try {
        // TODO: change the settings configurable
        spdlog::init_thread_pool(QUEUE_LENGTH, 1);
        std::vector<spdlog::sink_ptr> sinks;

        // To console
        if (config.toConsole) {
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            console_sink->set_level(LEVEL_MAP.at(config.level));
            sinks.push_back(console_sink);
        }

        // To file
        if (config.toFile) {
            auto logDir = config.logDir;
            if (!std::filesystem::exists(logDir)) {
                std::filesystem::create_directories(logDir);
            }
            auto file_sink = std::make_shared<spdlog::sinks::hourly_file_sink_mt>(
                logDir.string() + "/" + appName + ".log");
            // when writing to file, set the level to debug to avoid the overhead of logging
            file_sink->set_level(spdlog::level::debug);
            sinks.push_back(file_sink);
        }

        auto logger = std::make_shared<spdlog::async_logger>(appName, sinks.begin(), sinks.end(),
                                                             spdlog::thread_pool(),
                                                             spdlog::async_overflow_policy::block);
        logger->set_pattern(config.pattern);
        logger->set_level(spdlog::level::debug);
        logger->flush_on(spdlog::level::info);

        spdlog::register_logger(logger);
        spdlog::set_default_logger(logger);
        spdlog::flush_every(std::chrono::seconds(FLUSH_INTERVAL_SECONDS));

        // Redirect Qt messages to spdlog
        qInstallMessageHandler(qtMessageHandler);

        LOG_INFO("Logger initialized for {}", appName);
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize logger: " << e.what() << std::endl;
        return;
    }
}

void LogManager::shutdown() {
    spdlog::default_logger()->flush();

    qInstallMessageHandler(nullptr);
    LOG_INFO("Logger shutdown");
    spdlog::shutdown();
}