#pragma once

#include <QApplication>
#include <memory>

#include "thoth/Logger.h"

class AppRunner {
   public:
    AppRunner(int& argc, char** argv);
    ~AppRunner();

    int run();

   private:
    void initEnv();
    bool setupNetwork();
    bool authenticate();

    int& m_argc;
    char** m_argv;

    std::unique_ptr<QApplication> m_app;
    std::unique_ptr<LogManager::Guard> m_logGuard;
};
