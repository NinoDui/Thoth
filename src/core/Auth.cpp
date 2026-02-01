#include <cstdlib>
#include <vector>

#include "thoth/AuthProviderFactory.h"
#include "thoth/ConfigKey.h"
#include "thoth/ConfigStore.h"
#include "thoth/Logger.h"

#ifdef _WIN32
#include <stdlib.h>  // for _putenv_s
#endif

namespace fs = std::filesystem;

class IAuthStep {
   public:
    virtual ~IAuthStep() = default;
    virtual bool authenticate() = 0;

    virtual std::string getName() const = 0;

   protected:
    const std::string ENV_KEY_GOOGLE_APPLICATION_CREDENTIALS = "GOOGLE_APPLICATION_CREDENTIALS";

    void setEnv(const std::string& key, const std::string& value) {
#ifdef _WIN32
        _putenv_s(key.c_str(), value.c_str());
#else
        setenv(key.c_str(), value.c_str(), 1);
#endif
    }
};

class LocalConfigAuthStep : public IAuthStep {
   public:
    std::string getName() const override { return "LocalConfigAuthStep"; }

    bool authenticate() override {
        auto path = ConfigStore::instance().getValue<std::string>(
            thoth::config::KEY_GOOGLE_CREDENTIAL_PATH);
        if (!path) {
            LOG_DEBUG("Google Application Credentials not found in local config, skipping...");
            return false;
        }

        fs::path p(*path);
        if (isValid(p)) {
            setEnv(ENV_KEY_GOOGLE_APPLICATION_CREDENTIALS, p.string());
            return true;
        } else {
            LOG_DEBUG("Google Application Credentials file not valid: {}", p.string());
        }
        return false;
    }

   private:
    bool isValid(const fs::path& path) const {
        return fs::exists(path) && fs::is_regular_file(path);
    }
};

class FileRequestAuthStep : public IAuthStep {
   public:
    explicit FileRequestAuthStep(const FileRequestCallback& callback)
        : m_callback(std::move(callback)) {}

    std::string getName() const override { return "FileRequestAuthStep"; }

    bool authenticate() override {
        if (!m_callback) {
            LOG_WARN("FileRequestCallback is not set, skipping...");
            return false;
        }

        auto path = m_callback();

        if (isValid(path)) {
            setEnv(ENV_KEY_GOOGLE_APPLICATION_CREDENTIALS, path.string());
            ConfigStore::instance().setValue(thoth::config::KEY_GOOGLE_CREDENTIAL_PATH,
                                             path.string());
            return true;
        } else {
            LOG_DEBUG("Google Application Credentials file not valid: {}", path.string());
        }
        return false;
    }

   private:
    bool isValid(const fs::path& path) const {
        return !path.empty() && fs::exists(path) && fs::is_regular_file(path);
    }
    FileRequestCallback m_callback;
};

class LoginRequestAuthStep : public IAuthStep {
   public:
    explicit LoginRequestAuthStep(const LoginRequestCallback& callback)
        : m_callback(std::move(callback)) {}

    std::string getName() const override { return "LoginRequestAuthStep"; }

    bool authenticate() override {
        if (!m_callback) {
            LOG_WARN("LoginRequestCallback is not set, skipping...");
            return false;
        }
        throw std::runtime_error("LoginRequestCallback is not implemented");
        return false;
    }

   private:
    LoginRequestCallback m_callback;
};

class AuthChainImpl : public AuthChain {
   public:
    void addStep(std::unique_ptr<IAuthStep> step) { m_steps.push_back(std::move(step)); }

    bool execute() override {
        for (const auto& step : m_steps) {
            if (step->authenticate()) {
                return true;
            } else {
                LOG_DEBUG("Authentication for step {} failed", step->getName());
            }
        }
        return false;
    }

   private:
    std::vector<std::unique_ptr<IAuthStep>> m_steps;
};

std::unique_ptr<AuthChain> AuthProviderFactory::createAuthChain(const AuthCallbacks& callbacks) {
    auto chain = std::make_unique<AuthChainImpl>();

    chain->addStep(std::make_unique<LocalConfigAuthStep>());
    if (callbacks.onRequestFile) {
        chain->addStep(std::make_unique<FileRequestAuthStep>(callbacks.onRequestFile));
    }
    if (callbacks.onRequestLogin) {
        chain->addStep(std::make_unique<LoginRequestAuthStep>(callbacks.onRequestLogin));
    }
    return chain;
}