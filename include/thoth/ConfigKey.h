#pragma once

namespace thoth::config {
constexpr const char* KEY_CACHE_DIR = "general/cache_dir";
constexpr const char* KEY_LOG_DIR = "general/log_dir";
constexpr const char* KEY_GOOGLE_CREDENTIAL_PATH = "general/google_credential_path";

constexpr const char* KEY_TTS_LANG = "tts/language_code";
constexpr const char* DEFAULT_TTS_LANG = "en-US";
constexpr const char* KEY_TTS_VOICE = "tts/voice_name";
constexpr const char* DEFAULT_TTS_VOICE = "en-US-Wavenet-D";
constexpr const char* KEY_TTS_AUDIO_ENCODING = "tts/audio_encoding";
constexpr const char* DEFAULT_TTS_AUDIO_ENCODING = "MP3";

constexpr const char* KEY_LOG_LEVEL = "log/level";
constexpr const char* DEFAULT_LOG_LEVEL = "debug";
constexpr const char* KEY_LOG_PATTERN = "log/pattern";
constexpr const char* DEFAULT_LOG_PATTERN = "[%Y-%m-%d %H:%M:%S.%e][%t][%^%l%$] %s:%# %! %v";
constexpr const char* KEY_LOG_TO_CONSOLE = "log/to_console";
constexpr const bool DEFAULT_LOG_TO_CONSOLE = true;
constexpr const char* KEY_LOG_TO_FILE = "log/to_file";
constexpr const bool DEFAULT_LOG_TO_FILE = true;

constexpr const char* KEY_PROXY = "network/proxy";
constexpr const char* DEFAULT_PROXY_HOST = "127.0.0.1";
constexpr const int DEFAULT_PROXY_PORT = 7890;
}  // namespace thoth::config