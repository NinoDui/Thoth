#pragma once

#include <cstdint>

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

constexpr const char* KEY_TTS_ENGINE = "tts/engine";
constexpr const char* DEFAULT_TTS_ENGINE = "gcp";
constexpr const char* KEY_TTS_PIPER_MODEL_PATH = "tts/piper_model_path";
constexpr const char* DEFAULT_TTS_PIPER_MODEL_PATH = "models/piper/en_US-lessac-medium.onnx";

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

constexpr const char* KEY_AUDIO_RECORDER_SAMPLE_RATE = "audio/recorder_sample_rate";
constexpr const uint32_t DEFAULT_AUDIO_RECORDER_SAMPLE_RATE = 16000;
constexpr const char* KEY_AUDIO_RECORDER_CHANNELS = "audio/recorder_channels";
constexpr const uint16_t DEFAULT_AUDIO_RECORDER_CHANNELS = 1;
constexpr const char* KEY_AUDIO_RECORDER_SAMPLE_FORMAT = "audio/recorder_sample_format";
constexpr const uint16_t DEFAULT_AUDIO_RECORDER_SAMPLE_FORMAT = 16;
constexpr const char* KEY_AUDIO_RECORDER_BUFFER_SIZE = "audio/recorder_buffer_size";
constexpr const uint32_t DEFAULT_AUDIO_RECORDER_BUFFER_SIZE = 1024;
constexpr const char* KEY_AUDIO_RECORDER_RMS_STEP = "audio/recorder_rms_step";
constexpr const uint8_t DEFAULT_AUDIO_RECORDER_RMS_STEP = 8;

constexpr const char* KEY_WHISPER_MODEL_PATH = "whisper/model_path";
constexpr const char* DEFAULT_WHISPER_MODEL_PATH = "models/ggml-base.en.bin";
constexpr const char* KEY_WHISPER_MODEL_LANGUAGE = "whisper/model_language";
constexpr const char* DEFAULT_WHISPER_MODEL_LANGUAGE = "en";
constexpr const char* KEY_WHISPER_MODEL_NAME = "whisper/model_name";
constexpr const char* DEFAULT_WHISPER_MODEL_NAME = "large-v3-turbo";

constexpr const char* DEFAULT_TIME_FORMAT = "%Y%m%d%H%M%S";
}  // namespace thoth::config
