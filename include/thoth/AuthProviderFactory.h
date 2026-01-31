#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>

// pre-declare, pay attention to destructor!
class AuthChain;

using FileRequestCallback = std::function<std::filesystem::path()>;
using LoginRequestCallback = std::function<bool()>;

struct AuthCallbacks {
    // When the auth chain requires the user to set a file
    FileRequestCallback onRequestFile;
    // When the auth chain requires the user to login
    LoginRequestCallback onRequestLogin;
};

class AuthProviderFactory {
   public:
    static std::unique_ptr<AuthChain> createAuthChain(const AuthCallbacks& callbacks);
};

class AuthChain {
   public:
    virtual ~AuthChain() = default;

    virtual bool execute() = 0;
};