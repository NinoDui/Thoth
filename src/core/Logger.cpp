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
    try {
        // TODO: change the settings configurable
        spdlog::init_thread_pool(QUEUE_LENGTH, 1);
        std::vector<spdlog::sink_ptr> sinks;

        // To console
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::info);
        sinks.push_back(console_sink);

        // To file
        auto file_sink =
            std::make_shared<spdlog::sinks::hourly_file_sink_mt>("logs/" + appName + ".log");
        file_sink->set_level(spdlog::level::debug);
        sinks.push_back(file_sink);

        auto logger = std::make_shared<spdlog::async_logger>(appName, sinks.begin(), sinks.end(),
                                                             spdlog::thread_pool(),
                                                             spdlog::async_overflow_policy::block);
        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%t] [%s:%# %!] [%^%l%$] %v");
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