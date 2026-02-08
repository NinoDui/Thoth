#include "internal/Timer.h"
#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif

#if defined(__cpp_lib_format)
#include <chrono>
#include <format>  // C++20
#else
#include <ctime>
#include <iomanip>
#include <sstream>

#endif

#include "thoth/ConfigKey.h"

namespace thoth::internal {
std::string getCurrentTimestamp() {
#if defined(__cpp_lib_format)
    auto now = std::chrono::system_clock::now();
    return std::format("{:%Y%m%d%H%M%S}", now);
#else
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);

    std::stringstream ss;
    ss << std::put_time(&tm, thoth::config::DEFAULT_TIME_FORMAT.c_str());
    return ss.str();
#endif
}
}  // namespace thoth::internal