# Project Development Document: Thoth

**Version**: 0.2.0
**Last Updated**: 2026-05-03
**Status**: Active Development (MVP Phase, text-input shadowing path complete)

---

## 1. Project Overview

Thoth is a cross-platform (macOS / Windows) desktop GUI application for foreign-language shadowing practice, implemented in **C++20** with **Qt 6**. It helps learners improve pronunciation and fluency by importing text content, generating reference audio via TTS, recording user attempts, transcribing them with whisper.cpp, and scoring them with token-level WER comparison.

### 1.1 Product Vision

A configurable, local-first shadowing practice tool that supports multiple languages, integrates with both cloud and local AI services, and maintains industrial-grade engineering quality.

### 1.2 Key Differentiators

- **Local-first scoring**: whisper.cpp runs the user's recording locally — no audio leaves the machine for the scoring path.
- **Provider abstraction**: TTS engines are swapped behind `thoth::ITTSEngine`; scoring is swapped behind `ISentenceScorer`.
- **Live-reload configuration**: settings changes are applied without restart (TTS engine, voice, Whisper model, proxy).
- **Lock-free audio capture pipeline**: 16 kHz capture → SPSC ring buffer → IO-thread WAV writer, no mutex on the audio hot path.
- **Industrial-grade C++20**: RAII, smart pointers, dispatch via Qt signals + queued connections, Google-style code conventions, pre-commit + clang-format enforcement.

---

## 2. Technology Stack

| Layer | Technology | Version | Purpose |
|-------|-----------|---------|---------|
| Language | C++20 | ISO 14882:2020 | `std::format`, designated initializers, `<filesystem>` |
| GUI Framework | Qt 6 | 6.x | Widgets, Multimedia, Network, Concurrent, Test |
| Build System | CMake | ≥3.20 | Configuration & generation (Ninja) |
| Package Manager | vcpkg | manifest mode | C++ dependency resolution |
| Logging | spdlog | latest | Async logger, console + hourly-rotating file sinks |
| Config Format | nlohmann/json | latest | Hierarchical key/value config store |
| Cloud TTS | Google Cloud Text-to-Speech C++ SDK | latest | Online TTS synthesis |
| Local STT | whisper.cpp | git submodule | Local speech-to-text (model loaded lazily) |
| Local TTS | Piper | – | Engine adapter present; synthesis not yet implemented |
| Testing | GoogleTest + GMock + QSignalSpy | latest | Unit & integration tests |
| Code Style | clang-format, cmake-format, pre-commit | – | Style enforcement |

### 2.1 Compiler Requirements

- **macOS**: Apple Clang (Xcode Command Line Tools), arm64-osx triplet
- **Windows**: MSVC 2022 Build Tools

### 2.2 Build Presets (CMakePresets.json)

| Preset | Generator | Build Type | Binary Dir |
|--------|-----------|-----------|-----------|
| `debug` | Ninja | Debug | `build/debug` |
| `release` | Ninja | Release | `build/release` |

Toolchain wrapper at `cmake/ThothVcpkgToolchain.cmake` selects the platform triplet before vcpkg loads.

---

## 3. Architecture

### 3.1 Layered Architecture

```
┌─────────────────────────────────────────────────┐
│              ThothApp (executable)              │
│  src/app/  main.cpp · AppRunner · UI widgets    │
├─────────────────────────────────────────────────┤
│            ThothCore (static library)           │
│  ┌────────────┐  ┌────────────┐  ┌────────────┐ │
│  │ Public API │  │ Controllers│  │  Services  │ │
│  │ include/   │  │src/core/   │  │ src/core/  │ │
│  │  thoth/*.h │  │controllers/│  │  services/ │ │
│  └────────────┘  └────────────┘  └────────────┘ │
│  ┌────────────┐  ┌────────────┐  ┌────────────┐ │
│  │  Entities  │  │  Internal  │  │   Utils    │ │
│  │src/core/   │  │ src/core/  │  │ src/core/  │ │
│  │ entities/  │  │  internal/ │  │   utils/   │ │
│  └────────────┘  └────────────┘  └────────────┘ │
├─────────────────────────────────────────────────┤
│ Dependencies: Qt6 · GCP TTS · whisper.cpp ·     │
│               spdlog · nlohmann/json · gtest    │
└─────────────────────────────────────────────────┘
```

### 3.2 Design Principles

1. **Interface segregation** — Every replaceable subsystem (TTS, STT, scoring, content provider, audio storage, audio capture) is fronted by a pure-virtual interface in either `include/thoth/` or `src/core/internal/`.
2. **Public vs internal headers** — `include/thoth/*.h` is the supported API surface for both `ThothApp` and `ThothTests`. `src/core/internal/*.h` is private to the core library.
3. **Composition over inheritance** — Controllers own their workers and services as members; cross-cutting collaboration happens via Qt signals + slots.
4. **Thread safety via channels, not locks** — The capture→storage pipeline uses an SPSC lock-free ring buffer; cross-thread control uses `Qt::QueuedConnection`. The only lock is `ConfigStore::m_configMutex` around JSON read/write.
5. **Fail-fast validation** — Invalid WAV headers, missing models, and config errors throw `std::runtime_error` with descriptive messages. The 44-byte WAV header struct has a `static_assert` on its size.
6. **Live-reload by signal** — `ConfigStore::configChanged(QString)` is the single source of truth for runtime mutation; `Q_AppMainWindow` reconnects affected subsystems on receipt.

### 3.3 Thread Model

```
Main Thread (GUI)
  ├── Q_AppMainWindow
  │     ├── Q_SessionPlaybackController (drives Q_AudioPlayer)
  │     ├── Q_RecordController          (drives capture + IO worker)
  │     └── Q_ASRController             (drives Whisper worker)
  │
  ├── IO Worker Thread          ◄── owns AudioFileStreamSaver
  │     └── QTimer (30 ms) drains LockFreeRingBuffer → QFile (WAV)
  │
  ├── Whisper Worker Thread     ◄── owns Q_WhisperWorker (whisper_context*)
  │     └── slot doTranscribe(RecordedSentence*)
  │           WAV::decode → whisper_full → emit transcriptReady
  │
  └── QtConcurrent thread pool  ◄── short-lived TTS download tasks
        └── ITTSEngine::synthesize() → QFutureWatcher::finished (main thread)
```

**Synchronization**:
- `LockFreeRingBuffer` — SPSC, 1 MB capacity, cache-line-padded atomics, "waste one slot" full-detection.
- `ConfigStore::m_configMutex` — `std::mutex` around all `nlohmann::json` read/write.
- Cross-thread signals — `Qt::QueuedConnection` (default for objects in different threads); `RecordedSentence*` is passed by raw pointer because the caller (`Q_AppMainWindow`) owns the backing `std::vector<RecordedSentence>` for the session lifetime.

---

## 4. Module Design

### 4.1 Public API (`include/thoth/`)

| Header | Key Types | Purpose |
|--------|-----------|---------|
| `Entity.h` | `Sentence`, `RecordedSentence`, `Session`, `TimeRange` | Domain entities |
| `ITTSEngine.h` | `thoth::ITTSEngine`, `thoth::TTSAudioResult` | TTS provider interface (`synthesize`, `engineName`, `isAvailable`) |
| `ISentenceScorer.h` | `ISentenceScorer` | Scoring interface returning `[0.0, 1.0]` |
| `ContentProvider.h` | `IContentProvider`, `TextContentProvider` | Content loading + per-sentence audio preparation |
| `ConfigStore.h` | `ConfigStore` (Qt singleton) | Runtime config, paths, typed getters; emits `configChanged(QString)` |
| `ConfigKey.h` | `thoth::config::KEY_*` and `DEFAULT_*` constants | Compile-time config keys and defaults |
| `ThothConfig.h` | `AudioRecorderConfig`, `WhisperConfig`, `GoogleTTSConfig`, `LogConfig` | Typed POD config structs |
| `Logger.h` | `LogManager::Guard`, `LOG_*` macros | RAII logger lifecycle, spdlog-backed macros |
| `Q_ASRController.h` | `Q_ASRController` | ASR orchestration (owns `Q_WhisperWorker` on dedicated thread) |
| `Q_RecordController.h` | `Q_RecordController` | Recording lifecycle (capture → ring buffer → IO worker) |
| `Q_SessionPlaybackController.h` | `Q_SessionPlaybackController` | Sentence-by-sentence playback orchestration |
| `Q_AudioPlayer.h` | `Q_AudioPlayer` | Pure-functional `QMediaPlayer` + `QAudioOutput` wrapper |
| `WERScorer.h` | `WERScorer : ISentenceScorer` | WER-based scoring (Levenshtein on normalized tokens) |
| `TTSEngineFactory.h` | `thoth::CreateTTSEngine()` | Factory; selects Piper if requested + available, otherwise GCP |
| `AuthProviderFactory.h` | `AuthChain`, `AuthCallbacks`, `AuthProviderFactory` | Chain-of-responsibility GCP auth |

### 4.2 Core Library (`src/core/`)

#### 4.2.1 `common/`

| File | Purpose |
|------|---------|
| `ConfigStore.cpp` | JSON config at `~/.config/Thoth/config.json`. Thread-safe singleton. Provides typed getters (`getGoogleTTSConfig`, `getWhisperConfig`, `getAudioRecorderConfig`, `getLogConfig`) and path helpers (`getTempDir`, `getCacheDir`, `getLogDir`). Temp dir is `/tmp/Thoth/<launchTimestamp>` on POSIX, `<system-temp>/Thoth/<launchTimestamp>` on Windows; the timestamp is captured once at first call and reused for the process lifetime. |
| `Logger.cpp` | spdlog async logger (thread pool size 8192, single worker). Console (`stdout_color_sink_mt`) and hourly-rotating file (`hourly_file_sink_mt`) sinks. `qInstallMessageHandler` redirects Qt messages → spdlog. Flush every 5 s. |
| `Auth.cpp` | Chain-of-responsibility auth: `LocalConfigAuthStep` (cached path) → `FileRequestAuthStep` (UI prompt) → `LoginRequestAuthStep` (not implemented — throws). On success, sets `GOOGLE_APPLICATION_CREDENTIALS` and persists path to `ConfigStore`. |
| `LockFreeRingBuffer.cpp` | SPSC ring buffer. Cache-line aligned `std::atomic<size_t>` for read/write indices, acquire/release ordering. |

#### 4.2.2 `controllers/`

| Controller | Responsibility |
|------------|---------------|
| `Q_SessionPlaybackController` | Sentence playback loop: `play / playSentence / playNext / playPrevious / pause / resume / stop`. Single-sentence loop with configurable inter-loop delay (`QTimer`). Prefetches next 3 sentences via `IContentProvider::prepareAudio` with no-op callback. `suspendAutoAdvance()` lets external callers (e.g., user-audio playback) borrow `Q_AudioPlayer` without triggering `playNext()` on EndOfMedia. |
| `Q_RecordController` | Owns `Q_AudioCaptureProducer` + `LockFreeRingBuffer` + `AudioFileStreamSaver` (latter moved to dedicated `QThread`). Forwards mic data into ring buffer; emits `updateAmplitude(float)` for the visualizer; emits `recordingFinished(path)` on session finalize. Has a public DI constructor used by `TestRecordController`. |
| `Q_ASRController` | Owns `Q_WhisperWorker` on dedicated `QThread`. `analyze(RecordedSentence*)` dispatches via `sigTranscribe` (queued). On `transcriptReady`, computes `m_scorer->score(reference, hypothesis)`, writes back into `RecordedSentence::shadowingScore` (0.0–1.0), emits `analysisReady`. |
| `Q_WhisperWorker` (internal) | Lazy-loads whisper model on first transcription (`whisper_init_from_file_with_params`). Decodes recorded WAV → float PCM via `WAV::decode`, runs `whisper_full` greedy, concatenates segment text, trims leading space. `reloadModel()` frees the context so the next transcribe re-initializes. Emits `busyChanged(bool)` for UI gating. |

#### 4.2.3 `services/`

| Service | Purpose |
|---------|---------|
| `GCP.cpp` (`GCPTextToSpeechClient`) | Wraps `google::cloud::texttospeech_v1::TextToSpeechClient`. Maps the configured `audioEncoding` string to the GCP enum (MP3 / OGG_OPUS / LINEAR16 / MULAW; defaults to MP3). Throws `std::runtime_error` wrapping `google::cloud::Status` on failure. |
| `GCPTTSEngine.cpp` | `thoth::ITTSEngine` adapter over `GCPTextToSpeechClient`. `isAvailable()` returns `true` unconditionally; failures surface at `synthesize()` time. |
| `PiperTTSEngine.cpp` | Local Piper adapter. `isAvailable()` returns `true` if the model file exists, but `synthesize()` is a logged warning that returns an empty result — full Piper integration is deferred. |
| `TTSEngineFactory.cpp` | `CreateTTSEngine(engineName, gcpConfigPath, piperModelPath)`. If `engineName == "piper"` and the engine reports available, returns Piper; otherwise constructs `GCPTTSEngine` from `ConfigStore::getGoogleTTSConfig()`. The `gcpConfigPath` parameter is currently unused (call site passes `""`). |
| `Q_GCPTTSDownloader.cpp` (class `Q_TTSDownloader`) | Wraps any `ITTSEngine` for async dispatch via `QtConcurrent::run` + `QFutureWatcher`. Callback receives `(success, QByteArray, errorMsg)` on the main thread. |
| `TextContentProvider.cpp` | Implements `IContentProvider`: parses `.txt` via `TextParser`, builds `Session` with `Sentence` entries, then `prepareAudio()` checks `AudioCache`, deduplicates via `m_downloadingIdx`, and dispatches via `Q_TTSDownloader`. On success, writes file to cache (MD5-named) and stores path in `Sentence::localAudioPath`. |
| `AudioCache.cpp` | Per-sentence audio cache. Filename derived from **MD5** hex of the sentence text + `.mp3` extension. `get(sentence)` returns the cached path if the file exists and is non-empty. `saveAudio(idx, sentence, data)` writes via `QFile` and tracks `idx → path` map. |
| `AudioCapture.cpp` (`Q_AudioCaptureProducer`) | Wraps `QAudioSource` over the default input device. Negotiates format with the device; falls back to `defaultDevice.preferredFormat()` if the requested 16 kHz / mono / Int16 is not supported. Logs the negotiated format. Emits `audioDataAvailable(QByteArray)` on every `readyRead`. |
| `StreamAudioStorage.cpp` (`AudioFileStreamSaver`) | Drains `LockFreeRingBuffer` into a `QFile` every 30 ms. Writes a 44-byte placeholder header on `startSession`, accumulates `m_totalBytes`, and back-writes the real `WAVHeader` on `finalizeSession`. Default root is `<cacheDir>/record/`; session file is `<sessionId>.wav`. |
| `Q_AudioPlayer.cpp` | Thin `QMediaPlayer` + `QAudioOutput` wrapper (path → playback). Translates `QMediaPlayer::MediaStatus` and error events into typed signals (`started / paused / resumed / stopped / finished / errorOccurred`). |

#### 4.2.4 `entities/`

| Entity | Purpose |
|--------|---------|
| `WAV.cpp` | `WAVHeader::create / read / isValid`, `WAV::decode` (file or stream). Validates header (PCM only, channels ≥ 1, dataSize % blockAlign == 0). Decodes 8/16/32-bit PCM into normalized float samples (mono-mixdown). `WAV::resample` is a stub. |

#### 4.2.5 `utils/`

| Utility | Purpose |
|---------|---------|
| `Segmenters.cpp` (`RegexSegmenter`) | Splits on `[.?!\n]\s+|$`. Filters out all-digit and all-punctuation tokens. English-tuned. |
| `TextParser.cpp` | Composes `FileExtractor` + `Segmenter` for the `path → sentences` pipeline. PIMPL-style. |
| `FileExtractors.cpp` | `TxtExtractor` (read full file). `PdfExtractor` is a stub. Factory by extension or format string. |
| `WERScorer.cpp` | Lowercase + replace punctuation with spaces, tokenize on whitespace, Levenshtein on token vectors. Returns `max(0, 1 - min(1, WER))`. Empty reference + empty hypothesis → 1.0; empty reference + non-empty hypothesis → 0.0. |
| `AudioTools.cpp` | `QAudioSampleFormatToBits()` — Qt `QAudioFormat::SampleFormat` → bit depth. |
| `Timer.cpp` | `getCurrentTimestamp()` formatted via `std::format` for session and temp-dir naming. |

#### 4.2.6 `internal/` headers

| Header | Key Declarations |
|--------|------------------|
| `InternalEntity.h` | `WAVHeader` (packed, exactly 44 bytes — `static_assert`), `WAV` |
| `gcp.h` / `GCPTTSEngine.h` | GCP TTS client + `ITTSEngine` adapter |
| `PiperTTSEngine.h` | Piper adapter |
| `Segmenter.h` | `Segmenter` interface + `RegexSegmenter` + factory |
| `TextParser.h` | `TextParser` PIMPL with injectable `Segmenter` |
| `FileExtractor.h` | `FileExtractor` interface + factories |
| `Q_AudioCapture.h` | `IAudioCaptureService` + `Q_AudioCaptureProducer` (Qt + interface multi-inheritance) |
| `Q_AudioStorage.h` | `IStreamStorage`, `Q_AudioStreamConsumer` (abstract Qt), `AudioFileStreamSaver` |
| `Q_WhisperWorker.h` | Whisper worker (`doTranscribe`, `reloadModel` slots) |
| `Q_GCPTTSDownloader.h` | `Q_TTSDownloader` async wrapper (alias `Q_GCPTTSDownloader`) |
| `AudioCache.h` | MD5-hashed audio cache |
| `LockFreeRingBuffer.h` | SPSC ring buffer |
| `Timer.h`, `Utils.h` | Helpers |

### 4.3 Application Layer (`src/app/`)

| File | Purpose |
|------|---------|
| `main.cpp` | Constructs `AppRunner` and calls `run()` — that's the entire entry point. |
| `AppRunner.{h,cpp}` | Bootstrap order: `ConfigStore::init()` → `LogManager::Guard` → `QApplication` → `StyleLoader::attach()` → `setupNetwork()` (proxy env vars) → `authenticate()` (chain prompts for GCP credential) → `Q_AppMainWindow::show()` → `app->exec()`. Top-level try/catch around `run()` translates uncaught exceptions to `QMessageBox::critical`. |
| `ui/Q_AppMainWindow.{h,cpp}` | Main window: menu bar (`Import File`, `Settings`, `Exit`), centered status label, `QListWidget` of sentences (with bold-highlight of currently-playing item and `[score%]` badge after analysis), `Q_PlaybackControlBar`, `Q_ShadowingBar`. Constructs all controllers in `setupControllers()`. Wires UI signals to controllers in `setup{Playback,Recording,ASR}Connections()`. Reacts to `ConfigStore::configChanged` by re-creating the TTS engine (and rebinding playback connections) or reloading the Whisper model in place. |
| `ui/Q_SettingDialog.{h,cpp}` | Tabbed dialog (`General` / `Network` / `About`). General tab: cache dir, log dir, TTS engine combo (`gcp` / `piper`), GCP credential file picker, language combo (`en-US`, `zh-CN`, `ja-JP`), voice combo (editable), Piper model path, Whisper model path, read-only display of the config file location. Network tab: proxy string. Save writes through `ConfigStore::setValue` and pushes proxy env vars immediately. |
| `ui/Q_ShadowingBar.{h,cpp}` | Record/Stop toggle button, "Play user audio" button, RMS visualizer (`QProgressBar`), analyzing/result labels. Emits `sigStartRecording / sigStopRecording / sigPlayUserAudio`. Slots for `setAmplitude`, `onRecordingFinished`, `onASRAnalysisBusy`, `onASRAnalysisReady(scorePercent)`. |
| `ui/Q_PlaybackControlBar.{h,cpp}` | Prev / Play-Pause / Next buttons, single-sentence loop checkbox, inter-loop delay spinbox. |
| `ui/StyleLoader.h` | Stylesheet loader. |

### 4.4 Data Flow: Text-Input Shadowing

```
User imports .txt
    │
    ▼
TextContentProvider::load
    └── TextParser::parseFile
          ├── TxtExtractor::extract
          └── RegexSegmenter::segment       → Session{ sentences[] }
    │
    ▼
Q_SessionPlaybackController::playSentence(idx)
    └── TextContentProvider::prepareAudio
          ├── AudioCache::get               → cache hit?  use path
          └── Q_TTSDownloader::download     → cache miss, run on QtConcurrent
                └── ITTSEngine::synthesize  → bytes → AudioCache::saveAudio (MD5)
    │
    ▼
Q_AudioPlayer::play(localAudioPath)
    └── _prefetchNext: prepare next 3 sentences  (warm cache)
    │
    ▼
User clicks Record (Q_ShadowingBar)
    │
    ▼
Q_RecordController::startRecording(sessionId)
    ├── Q_AudioCaptureProducer::start              → QAudioSource → QIODevice
    └── on audioDataAvailable
          ├── calculateRMS → emit updateAmplitude
          └── LockFreeRingBuffer::write (SPSC, no lock)
                                                    │ (IO worker thread)
                                                    ▼
                                       AudioFileStreamSaver::_drainBufferToFile
                                       (QTimer, 30 ms) → QFile (WAV)
    │
    ▼
User clicks Stop → finalizeSession → recordingFinished(path)
    │
    ▼
Q_ASRController::analyze(&recordedSentence)
    └── sigTranscribe (queued) → Whisper worker thread
          ├── WAV::decode                  → float PCM
          └── whisper_full(GREEDY)         → segment text → RecordedSentence::transcribedText
    │
    ▼
Q_ASRController::onTranscriptReady
    └── ISentenceScorer::score(reference, hypothesis)  → shadowingScore
    │
    ▼
Q_AppMainWindow::analysisReady lambda
    ├── Q_ShadowingBar::onASRAnalysisReady(scorePercent)
    └── List item text updated with [score%] badge
```

---

## 5. Configuration System

### 5.1 ConfigStore

Thread-safe Qt singleton at `~/.config/Thoth/config.json`.

- **Keys** — hierarchical strings (`tts/engine`, `whisper/model_path`, `audio/recorder_sample_rate`, …) defined as `constexpr const char*` in `ConfigKey.h`.
- **Generic accessors** — `template<T> setValue/getValue` over the JSON tree; `setValue` saves and emits `configChanged(QString)`.
- **Typed accessors** — `getGoogleTTSConfig`, `getWhisperConfig`, `getAudioRecorderConfig`, `getLogConfig` return populated POD structs from `ThothConfig.h`. Each falls back to defaults from `ConfigKey.h` per field.
- **Path helpers** — `getTempDir`, `getCacheDir = getTempDir/cache`, `getLogDir = getTempDir/log` (overridable via `KEY_LOG_DIR`), `getConfigFilePath`, `getConfigDir`. Temp dir is `/tmp/Thoth/<launchTimestamp>` (POSIX) or `<system-temp>/Thoth/<launchTimestamp>` (Windows); `<launchTimestamp>` is fixed for the process lifetime.

### 5.2 Key Configuration Entries

| Category | Key | Default | Notes |
|----------|-----|---------|-------|
| General | `general/cache_dir` | `<temp>/cache` | Base for audio cache (`audio/<engine>/`) and recordings (`record/`) |
| General | `general/log_dir` | `<temp>/log` | Log output directory |
| General | `general/google_credential_path` | – | Path to GCP service-account JSON; persisted by auth chain |
| TTS | `tts/engine` | `gcp` | `gcp` or `piper` |
| TTS | `tts/language_code` | `en-US` | GCP language tag |
| TTS | `tts/voice_name` | `en-US-Wavenet-D` | GCP voice model |
| TTS | `tts/audio_encoding` | `MP3` | `MP3` / `OGG_OPUS` / `WAV` (LINEAR16) / `MULAW` |
| TTS | `tts/piper_model_path` | `models/piper/en_US-lessac-medium.onnx` | Piper ONNX model |
| Network | `network/proxy` | – (host `127.0.0.1`, port `7890` advisory) | Sets `http_proxy` / `https_proxy` / `grpc_proxy` env vars |
| Audio | `audio/recorder_sample_rate` | `16000` | Hz |
| Audio | `audio/recorder_channels` | `1` | mono |
| Audio | `audio/recorder_sample_format` | `16` | bits (8/16/32/64) |
| Audio | `audio/recorder_buffer_size` | `1024` | bytes (advisory) |
| Audio | `audio/recorder_rms_step` | `8` | sample stride for RMS visualizer |
| Whisper | `whisper/model_path` | `models/ggml-base.en.bin` | Loaded lazily on first transcription |
| Whisper | `whisper/model_language` | `en` | Whisper language tag |
| Log | `log/level` | `debug` | console minimum (file sink is always `debug`) |
| Log | `log/pattern` | `[%Y-%m-%d %H:%M:%S.%e][%t][%^%l%$] %s:%# %! %v` | spdlog pattern |
| Log | `log/to_console`, `log/to_file` | `true`, `true` | sink toggles |

### 5.3 Live-Reload Behavior

`Q_AppMainWindow::onConfigChanged` switches on the changed key:

- `tts/*` (engine, voice, language, encoding, piper model path) → recreate `ITTSEngine`, recreate `TextContentProvider`, recreate `Q_SessionPlaybackController` with the existing `Q_AudioPlayer`, rebind playback connections, restore the active session.
- `whisper/model_path`, `whisper/model_language` → `Q_ASRController::reloadModel(...)` which queues `Q_WhisperWorker::reloadModel` on the worker thread (frees `whisper_context*`; next `doTranscribe` reloads).
- `network/proxy` → re-export proxy env vars in-process.

---

## 6. Build System

### 6.1 Build Targets (CMakeLists.txt)

| Target | Type | Defined In | Description |
|--------|------|------------|-------------|
| `ThothCore` | Static library | `src/core/CMakeLists.txt` | All business logic. `GLOB_RECURSE` over `src/core/*.cpp` (re-run CMake when adding files). |
| `ThothApp` | Executable | `src/app/CMakeLists.txt` | GUI application |
| `ThothTests` | Executable | `test/CMakeLists.txt` | GoogleTest binary; `gtest_discover_tests` registers each TEST with CTest (timeout 30 s) |
| `RunThothTest` | Custom | `test/CMakeLists.txt` | `ctest --output-on-failure` |
| `ThothDocs` | Custom | root | Doxygen target (working dir `docs/`) |
| `ThothFormat` | Custom | root | clang-format over `src/**/*.{h,cpp}` and `include/**/*.h` |
| `ThothClean` | Custom | root | `cmake -E remove_directory ${CMAKE_BINARY_DIR}` |

### 6.2 Dependency Management

- **vcpkg** (manifest mode, `vcpkg.json`): `nlohmann-json`, `spdlog`, `google-cloud-cpp[texttospeech]`, `gtest`. `VCPKG_ROOT` must be set at CMake configure time; toolchain wrapper at `cmake/ThothVcpkgToolchain.cmake` selects platform triplet.
- **Qt 6** — system-installed (Homebrew on macOS at `/opt/homebrew/opt/qtbase`, apt on Linux, Qt installer on Windows). Components: `Core`, `Concurrent`, `Widgets`, `Network`, `Multimedia`, `Test`.
- **whisper.cpp** — git submodule at `third_party/whisper.cpp`. Built as part of the project with examples / tests / server disabled. Update via `git submodule update --remote`.

### 6.3 Compiler Flags

- `-Wall -Wextra -pedantic` on GCC/Clang via `thoth_set_warning()` helper; `/W4` on MSVC. Applied to `ThothCore`.
- `CMAKE_AUTOMOC / AUTOUIC / AUTORCC` on. `CMAKE_EXPORT_COMPILE_COMMANDS` on (used by `.clangd`).

---

## 7. Testing

### 7.1 Framework

- GoogleTest + GMock + QSignalSpy
- `test_main.cpp` provides a `QCoreApplication` for tests that need the Qt event loop (signal-spy tests, `Q_RecordController` integration).
- `TEST_RESOURCE_DIR` compile definition points at `test/resources/`.

### 7.2 Test Coverage

| Test File | Module Under Test | Approx. Cases | Coverage |
|-----------|-------------------|--------------|----------|
| `WAVTest.cpp` | `WAVHeader`, `WAV::decode` | ~7 | header create/validate, 8-bit and 16-bit PCM decode, error paths |
| `WERScorerTest.cpp` | `WERScorer` | ~9 | exact match, total mismatch, partial, case insensitivity, punctuation normalization, empty inputs, score clamp |
| `TextParserTest.cpp` | `TextParser` + `RegexSegmenter` | ~2 | empty file error, short article segmentation |
| `TestRecordController.cpp` | `Q_RecordController` | ~4 | start/stop flow, audio data pipeline through ring buffer, session finalization (uses `MockAudioCaptureProducer` + `MockAudioFileStreamSaver`) |
| `TestLockFreeRingBuffer.cpp` | `LockFreeRingBuffer` | ~4 | basic read/write, full-capacity behavior, wrap-around, SPSC pressure test (~10 MB) |
| `GCPRequestTest.cpp` | (online smoke test) | – | excluded from normal CI runs |

### 7.3 Running Tests

```bash
cmake --build build/debug --target ThothTests
cd build/debug && ctest --output-on-failure
# or
cmake --build build/debug --target RunThothTest
```

---

## 8. Implementation Status vs PRD

### 8.1 Completed

| PRD | Feature | Status |
|-----|---------|--------|
| FR-002 | TXT file import | ✅ |
| FR-008 | Text segmentation | ✅ `RegexSegmenter` (English-tuned) |
| FR-012 | Google TTS | ✅ via google-cloud-cpp |
| FR-013 | Local + online TTS | ⚠️ GCP works; Piper adapter present but synthesis stub |
| FR-014 | Sentence-level TTS | ✅ |
| FR-015 | TTS caching | ✅ MD5-hashed file cache |
| FR-016 | TTS provider config | ✅ JSON config + live reload |
| FR-022 | Full playback (sentence-by-sentence) | ✅ |
| FR-023 | Single-sentence playback | ✅ |
| FR-024 | Interval playback | ✅ Configurable inter-loop delay |
| FR-025 | Playback controls | ✅ Play / Pause / Prev / Next / Loop / Delay (no speed control) |
| FR-027 | User recording | ✅ via `QAudioSource` |
| FR-028 | Sentence-level recording | ✅ |
| FR-029 | Recording storage | ✅ WAV files in `<cacheDir>/record/` |
| FR-030 | Manual stop | ✅ |
| FR-033 | Pronunciation scoring | ✅ WER-based (0–100% in UI badge) |
| FR-034 | Whisper.cpp integration | ✅ Local STT for scoring |
| NFR-006 | Async / non-blocking GUI | ✅ QtConcurrent for TTS, `QThread` workers for IO and Whisper |
| NFR-007 | Error handling | ✅ Exceptions + Qt error signals + `QMessageBox` for playback errors |
| NFR-010 | Modular architecture | ✅ 15 public headers, internal/services/controllers/utils split |
| NFR-011 | Provider abstraction | ✅ `ITTSEngine`, `ISentenceScorer`, `IContentProvider`, `IAudioCaptureService`, `IStreamStorage` |
| NFR-012 | Config-based design | ✅ JSON `ConfigStore` |
| NFR-013 | Project structure | ✅ separate `src/core`, `src/app`, `test`, `docs`, `models`, `third_party`, `cmake` |
| NFR-014 | Unit tests | ✅ 5 active test files |
| NFR-016 | API key handling | ✅ env var + auth chain + config-stored path |
| NFR-017 | Local data privacy | ✅ recordings, transcripts, sessions are all local |

### 8.2 In Progress / Partial

| PRD | Feature | Status / Note |
|-----|---------|--------------|
| FR-001 | Text-box input | ❌ Only file import via menu |
| FR-003 | Audio file import | ❌ `IContentProvider` only has `TextContentProvider` |
| FR-005 | Multi-language | ⚠️ Config + UI offer `en-US`, `zh-CN`, `ja-JP`; segmentation rules are English-tuned |
| FR-007 | Language detection | ❌ |
| FR-018 | Whisper.cpp as primary STT | ✅ for scoring path; not yet for source-audio transcription |
| FR-020/021 | Timestamp extraction + audio-text alignment | ❌ Whisper params currently disable timestamps |
| FR-031 | Pause-and-record mode | ❌ |
| FR-032 | Simultaneous shadowing mode | ⚠️ Possible by user discipline; no explicit mode |
| FR-036 | Word-level difference highlighting | ⚠️ Score badge only; no per-word marking yet |
| FR-038 | Session summary | ❌ |
| FR-029 | Session persistence across restarts | ❌ Sessions live in memory only |
| GUI-003 | Settings panel — playback speed | ❌ Not exposed |
| GUI-003 | Settings panel — sentence interval duration | ✅ Inter-loop delay spinbox in playback bar (not in Settings dialog) |

### 8.3 Not Started

| Feature | Priority |
|---------|----------|
| Audio file import → STT → segmentation → time-aligned playback | High (FR-003, FR-021) |
| Swedish language support (config + segmentation) | High (FR-005) |
| Word-level diff highlighting in sentence list | High (FR-036) |
| Session persistence / recovery | Medium (NFR-009) |
| Playback speed control | Medium |
| Session summary view | Medium (FR-038) |
| Piper synthesis implementation | Medium |
| WAV resampling (`WAV::resample` is a stub) | Medium |
| Real-time waveform visualization (current is RMS bar only) | Low |
| Exportable practice reports | Low |
| Spaced repetition for weak sentences | Low |
| **Silence-detection auto-stop for recording** | **Low** |
| **Japanese / Korean character-level scoring + tokenization** | **Low** |

---

## 9. Error Handling Strategy

### 9.1 Synchronous (`std::runtime_error`)

- File open / read failures (TXT, WAV, config)
- WAV header validation (size, PCM-only, channel count, dataSize % blockAlign)
- GCP TTS call failures — `google::cloud::Status` is wrapped into `runtime_error` with code + message because `Status` is not a `std::exception`.
- Unsupported file formats from `FileExtractor` factory
- Unsupported PCM bit depth in `_wavToFloat`

### 9.2 Asynchronous (Qt signals)

- `Q_AudioCaptureProducer::errorOccurred(QString)` → `Q_RecordController` re-emits → main window logs and resets shadowing bar
- `AudioFileStreamSaver::errorOccurred / sessionAborted` → `Q_RecordController` slot → forwarded as `errorOccurred`
- `Q_WhisperWorker::errorOccurred` → `Q_ASRController::errorOccurred` → main window logs
- `Q_AudioPlayer::errorOccurred` → `Q_SessionPlaybackController::_onPlayerError` → `errorOccurred` → main window opens a `QMessageBox` with an "Open Settings" action (typical cause is GCP download failure due to proxy)
- `Q_TTSDownloader` failures → callback receives `(false, QByteArray(), errorMsg)`

### 9.3 Recovery Behavior

- TTS failure: error dialog with shortcut to Settings; user can retry by replaying the sentence
- Whisper model load failure: logged; transcription returns and `shadowingScore = 0.0`
- Audio capture failure: shadowing bar resets to idle
- Config file missing: `m_config` stays as empty `nlohmann::json::object()`; defaults apply on every typed getter

---

## 10. Known Limitations

1. **Audio import not wired** — only `.txt` ingest is implemented; no `.mp3 / .wav / .m4a / .flac` reader.
2. **Single Whisper context** — model switch requires `reloadModel()` (free + lazy reinit on next transcribe).
3. **`WAV::resample` is a stub** — input must already match the recorded WAV's sample rate; recording defaults to 16 kHz which Whisper expects.
4. **No session persistence** — close = lose. Recordings remain on disk under `<cacheDir>/record/` but aren't reattached.
5. **Segmenter is English-tuned** — `RegexSegmenter` splits on `.?!\n`; CJK punctuation is not handled.
6. **No JP/KR token-level scoring** — WER tokenizes on whitespace, which is wrong for Japanese/Korean. (Low priority per current scope.)
7. **Piper not functional** — `synthesize()` returns empty even when `isAvailable()` is true.
8. **GCP is the de-facto TTS** — without a working Piper, network is required for new sentences (cached audio still plays offline).
9. **No silence-detection auto-stop** — recording runs until the user clicks Stop. (Low priority per current scope.)
10. **No playback speed UI**.
11. **Settings dialog does not expose Whisper language** (only the model path), even though `KEY_WHISPER_MODEL_LANGUAGE` is read by the engine.

---

## 11. Coding Conventions

### 11.1 Naming

| Category | Convention | Example |
|----------|-----------|---------|
| Classes / structs | `PascalCase` | `Q_SessionPlaybackController` |
| Public free functions | `PascalCase` | `CreateTTSEngine` |
| Methods | `camelCase` | `playSentence`, `prepareAudio` |
| Private members | `m_camelCase` | `m_currentIdx` |
| Private methods | `_camelCase` | `_onPlayerFinished`, `_drainBufferToFile` |
| Constants | `UPPER_SNAKE_CASE` | `DEFAULT_TTS_ENGINE`, `DRAIN_INTERVAL_MS` |
| Config-key namespace | `snake_case` | `thoth::config` |
| File names | `PascalCase.{h,cpp}` | `WERScorer.cpp` |

### 11.2 Qt Conventions

- Qt-derived widgets: `Q_` prefix (`Q_AppMainWindow`, `Q_SettingDialog`, `Q_ShadowingBar`).
- Internal `QObject`-derived helpers also use `Q_` prefix and live in `src/core/internal/`.
- PIMPL idiom for `TextContentProvider`, `Q_TTSDownloader`, etc., to keep public headers slim.
- Cross-thread dispatch goes through `signals` + `Qt::QueuedConnection`; `RecordedSentence*` is passed by raw pointer with the caller guaranteeing lifetime.

### 11.3 Style Enforcement

- `.clang-format` (Google-based)
- `.cmake-format.yaml`
- `.pre-commit-config.yaml` runs clang-format + cmake-format on every commit
- `thoth_set_warning()` helper applies `-Wall -Wextra -pedantic` (or `/W4`)

### 11.4 Documentation

- Doxygen target (`ThothDocs`) for API documentation generation (output configured by `docs/Doxyfile`).
- Public headers carry `@brief` comments where intent is non-obvious (e.g., `IContentProvider`, `Q_AudioPlayer`, `LockFreeRingBuffer`).
- Hot-path or subtle code (e.g., `LockFreeRingBuffer` "waste one slot" strategy, capture-format negotiation) carries inline comments explaining the *why*.

---

## 12. Third-Party Dependencies

### 12.1 Direct

| Library | License | Usage |
|---------|---------|-------|
| Qt 6 | LGPLv3 / Commercial | GUI, audio, networking, threads |
| Google Cloud C++ SDK (texttospeech) | Apache 2.0 | Cloud TTS |
| whisper.cpp | MIT | Local STT |
| spdlog | MIT | Logging |
| nlohmann/json | MIT | Configuration |
| GoogleTest / GMock | BSD-3-Clause | Testing |

### 12.2 Submodule

`third_party/whisper.cpp` — pinned commit, built in-tree with `WHISPER_BUILD_EXAMPLES / TESTS / SERVER` set OFF.

---

## 13. Directory Reference

```
Thoth/
├── cmake/
│   └── ThothVcpkgToolchain.cmake         # Triplet selection wrapper
├── include/thoth/                        # Public API (15 headers)
│   ├── Entity.h
│   ├── ITTSEngine.h
│   ├── ISentenceScorer.h
│   ├── ContentProvider.h
│   ├── ConfigStore.h
│   ├── ConfigKey.h
│   ├── ThothConfig.h
│   ├── Logger.h
│   ├── Q_ASRController.h
│   ├── Q_RecordController.h
│   ├── Q_SessionPlaybackController.h
│   ├── Q_AudioPlayer.h
│   ├── WERScorer.h
│   ├── TTSEngineFactory.h
│   └── AuthProviderFactory.h
├── src/
│   ├── app/
│   │   ├── main.cpp
│   │   ├── AppRunner.{h,cpp}
│   │   └── ui/
│   │       ├── Q_AppMainWindow.{h,cpp}
│   │       ├── Q_SettingDialog.{h,cpp}
│   │       ├── Q_ShadowingBar.{h,cpp}
│   │       ├── Q_PlaybackControlBar.{h,cpp}
│   │       └── StyleLoader.h
│   └── core/
│       ├── common/         # ConfigStore, Logger, Auth, LockFreeRingBuffer
│       ├── controllers/    # Q_ASRController, Q_RecordController,
│       │                   # Q_SessionPlaybackController, Q_WhisperWorker
│       ├── entities/       # WAV
│       ├── internal/       # 13 internal headers
│       ├── services/       # TTS, downloader, audio capture, storage,
│       │                   # content provider, audio cache, audio player
│       └── utils/          # Segmenters, TextParser, FileExtractors,
│                           # WERScorer, AudioTools, Timer
├── test/                   # 5 GoogleTest files + test_main.cpp
│   └── resources/
├── docs/
│   ├── PRD.md
│   ├── PDD.md              # this file
│   ├── TASKS.md
│   ├── agents/             # Agent skill / triage docs
│   └── superpowers/        # Design specs and plans
├── models/                 # Whisper / Piper model files (user-supplied)
├── scripts/                # Build / setup helpers
├── third_party/whisper.cpp # Git submodule
├── CMakeLists.txt
├── CMakePresets.json       # debug, release
├── vcpkg.json              # nlohmann-json, spdlog, google-cloud-cpp, gtest
├── .clang-format
├── .cmake-format.yaml
├── .pre-commit-config.yaml
└── README.md
```

---

## 14. Key Architectural Decisions

### 14.1 C++20 + Qt 6

- `std::format`, designated initializers, `<filesystem>`, `std::optional`, concepts-ready compilers.
- Qt 6 brings cross-platform multimedia (`QMediaPlayer`, `QAudioSource`), threading, and a stable widget toolkit under LGPL.

### 14.2 Static Library for Core

- Forces a clean public API via `include/thoth/`.
- Lets `ThothTests` link the same code as `ThothApp` without a GUI dependency.
- Keeps the door open for headless CLI / server reuse.

### 14.3 SPSC Lock-Free Ring Buffer

- Audio capture (16 kHz × 1 ch × 16 bit ≈ 32 kB/s) handed to disk-IO without mutex contention.
- Cache-line padding on the read/write atomics avoids false sharing.
- "Waste one slot" makes the empty/full discrimination trivial.

### 14.4 JSON Config (nlohmann/json)

- Single-header, no extra dependency.
- Hierarchical key strings keep getters/setters generic.
- Mutex around the `nlohmann::json` object because library access is not internally atomic.

### 14.5 Per-Process Temp Dir Timestamp

- `getTempDir()` uses a static-initialized timestamp so every launch gets an isolated `/tmp/Thoth/<ts>/{cache,log}` tree without colliding with prior runs or requiring cleanup.

### 14.6 Live-Reload via `configChanged`

- Single signal acts as the source of truth for runtime config mutation.
- `Q_AppMainWindow` decides what to reconstruct (TTS engine + playback chain) vs. mutate in place (Whisper model, proxy env vars).

### 14.7 Pointer-Based Cross-Thread Payload

- `RecordedSentence*` flows main → Whisper → main. The owning `std::vector<RecordedSentence>` lives on `Q_AppMainWindow` for the session, so the pointer is valid across thread hops without copying audio metadata.

---

## 15. Future Roadmap

### Phase 2 — Audio Import & Multi-Language (current)

- [ ] Audio file import (`.mp3`, `.wav`, `.m4a`, `.flac`)
- [ ] Source-audio Whisper transcription with timestamp extraction (`print_timestamps = true`)
- [ ] Audio-text alignment for sentence-level reference playback (FR-021)
- [ ] Swedish language support (config + segmentation rules)
- [ ] Settings dialog: expose Whisper language; add playback speed control

### Phase 3 — Practice Modes & Feedback

- [ ] Pause-and-record mode (auto-pause after reference; user records, then auto-resume)
- [ ] Word-level diff highlighting in sentence list (correct / missing / extra / different)
- [ ] Session summary panel (avg score, weak words, duration)
- [ ] Session persistence (JSON serialization or SQLite)

### Phase 4 — Polish

- [ ] Piper TTS full implementation + voice model browser
- [ ] WAV resampling implementation (currently stub)
- [ ] Real-time waveform visualization (replace RMS bar)
- [ ] Doxygen-rendered API site (CI-published)
- [ ] Metal / Core ML acceleration for Whisper on macOS

### Backlog (Low Priority)

- Japanese / Korean character-level scoring strategy and segmentation rules
- Silence-detection auto-stop for recording
- Spaced repetition for weak sentences
- Exportable practice reports (PDF / HTML)
- Cloud sync of sessions

---

## 16. Development Commands

```bash
# Setup (macOS)
./scripts/setup-macos.sh
export VCPKG_ROOT=$HOME/vcpkg

# Configure and build
cmake --preset debug
cmake --build build/debug

# Run tests
cd build/debug && ctest --output-on-failure
# or
cmake --build build/debug --target RunThothTest

# Format code
cmake --build build/debug --target ThothFormat

# Generate documentation
cmake --build build/debug --target ThothDocs

# Clean build artifacts
cmake --build build/debug --target ThothClean

# Re-symlink compile_commands.json for clangd (after configure)
ln -sf debug/compile_commands.json build/compile_commands.json
```
