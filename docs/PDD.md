# Project Development Document: Thoth

**Version**: 0.6.1
**Last Updated**: 2026-05-03
**Status**: Active Development (P0+P1 complete — config batching, voice UX fixes, macOS bundle, UI polish, encoding-aware audio cache)

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
- **Industrial-grade C++20**: RAII, smart pointers, dispatch via Qt signals + queued connections, Google-style code conventions, pre-commit + clang-format + clang-tidy + cppcheck enforcement.
- **Multi-mode shadowing**: Normal, pause-and-repeat, and simultaneous shadowing modes with configurable playback speed (0.5x–2.0x).
- **Word-level scoring feedback**: Levenshtein DP back-trace produces per-token Correct/Missing/Different labels rendered as colored rich text in the sentence list.
- **Dual input modes**: text import (`.txt` + paste) with TTS audio, or audio import (`.wav`) with Whisper transcription and source-audio range playback — same shadowing UI for both.

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
| Static Analysis | clang-tidy, cppcheck, cpplint, shellcheck | – | Code quality enforcement (pre-commit hooks) |

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
  │     ├── slot doTranscribe(RecordedSentence*)       → scoring path
  │     ├── slot doTranscribeFile(QString)             → source-audio import path
  │     └── WAV::decode → whisper_full → emit transcriptReady / transcriptSegmentsReady
  │
  └── QtConcurrent thread pool  ◄── short-lived TTS download + GCP voice discovery tasks
        └── ITTSEngine::synthesize() → QFutureWatcher::finished (main thread)
        └── GCPTextToSpeechClient::listVoices() → QFutureWatcher::finished (main thread)
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
| `ISentenceScorer.h` | `ISentenceScorer`, `TokenLabel`, `TokenResult`, `ScoringResult` | Scoring interface: `score()` returns `[0.0, 1.0]`, `scoreDetail()` returns per-token alignment (Correct/Missing/Different) + extra tokens |
| `ContentProvider.h` | `IContentProvider`, `TextContentProvider` | Content loading (`load`, `loadFromText`) + per-sentence audio preparation |
| `AudioContentProvider.h` | `AudioContentProvider : QObject, IContentProvider` | Audio import: dispatches source-audio to Whisper via `Q_ASRController`, builds `Session` from timestamped transcript segments, `prepareAudio()` validates source file (no TTS) |
| `ConfigStore.h` | `ConfigStore` (Qt singleton) | Runtime config, paths, typed getters; emits `configChanged(QString)` |
| `ConfigKey.h` | `thoth::config::KEY_*` and `DEFAULT_*` constants | Compile-time config keys and defaults |
| `ThothConfig.h` | `AudioRecorderConfig`, `WhisperConfig`, `GoogleTTSConfig`, `LogConfig` | Typed POD config structs |
| `Logger.h` | `LogManager::Guard`, `LOG_*` macros | RAII logger lifecycle, spdlog-backed macros |
| `Q_ASRController.h` | `Q_ASRController` | ASR orchestration (owns `Q_WhisperWorker` on dedicated thread) |
| `Q_RecordController.h` | `Q_RecordController` | Recording lifecycle (capture → ring buffer → IO worker) |
| `Q_SessionPlaybackController.h` | `Q_SessionPlaybackController` | Sentence-by-sentence playback orchestration |
| `Q_AudioPlayer.h` | `Q_AudioPlayer` | Pure-functional `QMediaPlayer` + `QAudioOutput` wrapper |
| `WERScorer.h` | `WERScorer : ISentenceScorer` | WER-based scoring with Levenshtein DP back-trace for per-token alignment |
| `TTSEngineFactory.h` | `thoth::CreateTTSEngine()` | Factory; selects Piper if requested + available, otherwise GCP |
| `Q_GCPVoiceLoader.h` | `Q_GCPVoiceLoader` | Async voice discovery via `QtConcurrent::run` → `GCPTextToSpeechClient::listVoices()`; emits `voicesReady` or `loadError` |
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
| `Q_SessionPlaybackController` | Sentence playback loop. For text/TTS sentences: calls `IContentProvider::prepareAudio` → `Q_AudioPlayer::play(path)`. For audio-import sentences with `audioRange`: calls `play(path, startMs, endMs)`. Prefetches next 3 sentences. `suspendAutoAdvance()`, shadowing modes, `repeatRequested` signal for pause-and-repeat. |
| `Q_RecordController` | Owns `Q_AudioCaptureProducer` + `LockFreeRingBuffer` + `AudioFileStreamSaver` (latter moved to dedicated `QThread`). Forwards mic data into ring buffer; emits `updateAmplitude(float)` for the visualizer; emits `recordingFinished(path)` on session finalize. Has a public DI constructor used by `TestRecordController`. |
| `Q_ASRController` | Owns `Q_WhisperWorker` on dedicated `QThread`. `analyze(RecordedSentence*)` dispatches scoring path via `sigTranscribe`; `transcribeFile(path)` dispatches source-audio path via `sigTranscribeFile`. On `transcriptReady`, calls `m_scorer->scoreDetail(reference, hypothesis)`, stores `ScoringResult` in `RecordedSentence::scoringDetail`, emits `analysisReady`. On `transcriptSegmentsReady`, emits timestamped `TranscriptSegment` list for `AudioContentProvider`. |
| `Q_WhisperWorker` (internal) | Lazy-loads whisper model on first transcription. **Scoring path** (`doTranscribe`): decodes recorded WAV → float PCM, runs `whisper_full` greedy, concatenates segment text. **Source-audio path** (`doTranscribeFile`): decodes WAV (resamples to 16 kHz if needed) or MP3/M4A/FLAC (via `Q_AudioDecoder::decodeToMono16kFloat`), sets `print_timestamps = true`, emits `TranscriptSegment` list. Exposes `progressChanged(int)` via `whisper_progress_callback` trampoline. `reloadModel()` frees the context for lazy reinit. Emits `busyChanged(bool)` for UI gating. |

#### 4.2.3 `services/`

| Service | Purpose |
|---------|---------|
| `GCP.cpp` (`GCPTextToSpeechClient`) | Wraps `google::cloud::texttospeech_v1::TextToSpeechClient`. `synthesize()` maps the configured `audioEncoding` string to the GCP enum (MP3 / OGG_OPUS / LINEAR16 / MULAW; defaults to MP3). `listVoices(languageCode)` queries Google for available TTS voices, returning a `std::vector<GoogleVoiceInfo>` with name, language codes, and gender per voice. Throws `std::runtime_error` wrapping `google::cloud::Status` on failure. |
| `GCPTTSEngine.cpp` | `thoth::ITTSEngine` adapter over `GCPTextToSpeechClient`. `isAvailable()` returns `true` unconditionally; failures surface at `synthesize()` time. |
| `PiperTTSEngine.cpp` | Local Piper adapter. `isAvailable()` returns `true` if the model file exists, but `synthesize()` is a logged warning that returns an empty result — full Piper integration is deferred. |
| `TTSEngineFactory.cpp` | `CreateTTSEngine(engineName, gcpConfigPath, piperModelPath)`. If `engineName == "piper"` and the engine reports available, returns Piper; otherwise constructs `GCPTTSEngine` from `ConfigStore::getGoogleTTSConfig()`. The `gcpConfigPath` parameter is currently unused (call site passes `""`). |
| `Q_GCPTTSDownloader.cpp` (class `Q_TTSDownloader`) | Wraps any `ITTSEngine` for async dispatch via `QtConcurrent::run` + `QFutureWatcher`. Callback receives `(success, QByteArray, errorMsg)` on the main thread. |
| `TextContentProvider.cpp` | Implements `IContentProvider`: parses `.txt` via `TextParser`, parses pasted text via `TextParser::parseText()`, builds `Session` with `Sentence` entries, then `prepareAudio()` checks `AudioCache`, deduplicates via `m_downloadingIdx`, and dispatches via `Q_TTSDownloader`. On success, writes file to cache (MD5-named) and stores path in `Sentence::localAudioPath`. |
| `AudioCache.cpp` | Per-sentence audio cache. Filename derived from **MD5** hex of `(sentence text | engine cacheIdentity string)` + an encoding-aware extension (`.mp3` / `.ogg` / `.wav`). Extension is extracted from the engine's `cacheIdentity` (which includes the configured `audio_encoding`), so different encodings produce correctly-named cache files. `get(sentence, cacheIdentity)` returns the cached path if the file exists and is non-empty. `saveAudio(idx, sentence, cacheIdentity, data)` writes via `QFile` and tracks `idx → path` map. |
| `AudioCapture.cpp` (`Q_AudioCaptureProducer`) | Wraps `QAudioSource` over the default input device. Negotiates format with the device; falls back to `defaultDevice.preferredFormat()` if the requested 16 kHz / mono / Int16 is not supported. Logs the negotiated format. Emits `audioDataAvailable(QByteArray)` on every `readyRead`. |
| `StreamAudioStorage.cpp` (`AudioFileStreamSaver`) | Drains `LockFreeRingBuffer` into a `QFile` every 30 ms. Writes a 44-byte placeholder header on `startSession`, accumulates `m_totalBytes`, and back-writes the real `WAVHeader` on `finalizeSession`. Default root is `<cacheDir>/record/`; session file is `<sessionId>.wav`. |
| `Q_AudioPlayer.cpp` | Thin `QMediaPlayer` + `QAudioOutput` wrapper. `play(path)` for full playback; `play(path, startMs, endMs)` for range playback (seeks on `LoadedMedia`, auto-stops via `QTimer` at `endMs`, emits `finished()`). Exposes `setRate(qreal)` / `rate()` for playback speed control. |
| `AudioContentProvider.cpp` | Implements `IContentProvider`: `load(audioPath)` dispatches to `Q_ASRController::transcribeFile()`, `onTranscriptSegmentsReady` builds `Session` with timestamped `Sentence` entries (merging/splitting Whisper segments). `prepareAudio()` validates source file exists and `audioRange` is set — never calls TTS. `loadFromText()` is a no-op. |

#### 4.2.4 `entities/`

| Entity | Purpose |
|--------|---------|
| `WAV.cpp` | `WAVHeader::create / read / isValid`, `WAV::decode` (file or stream). Validates header (PCM only, channels ≥ 1, dataSize % blockAlign == 0). Decodes 8/16/32-bit PCM into normalized float samples (mono-mixdown). `WAV::resample` implemented (linear interpolation to target sample rate). |

#### 4.2.5 `utils/`

| Utility | Purpose |
|---------|---------|
| `Segmenters.cpp` (`RegexSegmenter`) | Splits on `[.?!\n]\s+|$`. Filters out all-digit and all-punctuation tokens. English-tuned. |
| `TextParser.cpp` | Composes `FileExtractor` + `Segmenter` for the `path → sentences` pipeline. Also provides `parseText(std::string_view)` for in-memory text (used by text-box input). PIMPL-style. |
| `FileExtractors.cpp` | `TxtExtractor` (read full file). `PdfExtractor` is a stub. Factory by extension or format string. |
| `WERScorer.cpp` | Lowercase + replace punctuation with spaces, tokenize on whitespace, Levenshtein on token vectors. `score()` returns `max(0, 1 - min(1, WER))`. `scoreDetail()` additionally back-traces the DP table via `alignTokens()` to produce per-token Correct / Missing / Different labels and a list of extra hypothesis tokens. |
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
| `Q_AudioDecoder.h` | `Q_AudioDecoder : QObject` — synchronous `QAudioDecoder` wrapper for MP3/M4A/FLAC decode via `decodeToMono16kFloat()` |
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
| `ui/Q_AppMainWindow.{h,cpp}` | Main window: neumorphic three-panel workspace matching the current UI sketch: top input panel with `LOAD FILE` / `TEXT TYPE` / `LOAD AUDIO` / `SETTINGS` pills and a text paste box, central `Script and Sentences` panel with `QListWidget` + word-level colored diff via `HtmlDelegate`, and bottom control panel combining `Q_ShadowingBar` + `Q_PlaybackControlBar`. `onImportFile` dispatches `.txt` → `TextContentProvider`, audio files → `AudioContentProvider`. Constructs both content providers and all controllers in `setupControllers()`. The `File` menu contains only `Settings` and `Exit`; file import is exclusively via the INPUT panel buttons. |
| `ui/Q_SettingDialog.{h,cpp}` | Tabbed dialog (`General` / `Network` / `About`). General tab: cache/log dirs, TTS engine combo (toggles GCP vs Piper rows), GCP credential, 4-language voice combo with async Google voice discovery (`Q_GCPVoiceLoader`), audio encoding selector (MP3/OGG_OPUS/WAV/MULAW), Piper model path, Whisper model path + language combo. Voice combo is a pure dropdown; on language change it immediately clears and shows "Loading voices..." until the async fetch completes, with a `DefaultVoiceForLanguage()` fallback if the dialog is closed before voices arrive. Network tab: proxy. All config changes use batch mode (`setValueSilent` / `endBatchUpdate`) to trigger a single engine recreation on save. |
| `ui/Q_ShadowingBar.{h,cpp}` | Record/Stop toggle button, "Play user audio" button, RMS visualizer (`QProgressBar`), analyzing/result labels. Emits `sigStartRecording / sigStopRecording / sigPlayUserAudio`. Slots for `setAmplitude`, `onRecordingFinished`, `onASRAnalysisBusy`, `onASRAnalysisReady(scorePercent)`. `triggerRecording()` public method for programmatic recording start (used by pause-and-repeat mode). |
| `ui/Q_PlaybackControlBar.{h,cpp}` | Prev / Play-Pause / Next buttons, single-sentence loop checkbox, inter-loop delay spinbox, speed slider (0.5x–2.0x), shadowing mode combo (`Normal` / `Pause-and-Repeat` / `Simultaneous`). |
| `ui/StyleLoader.h` | Aggregated stylesheet loader. Loads isolated QSS resources from `qss/base/`, `qss/components/`, and `qss/screens/` rather than embedding CSS in C++ code. |

#### 4.3.1 UI Outlook and Styling

The current desktop outlook follows `assets/UI diagram.heic` for structure and `assets/UIstyle.jpg` for visual language.

Layout:
- **Top input panel** — file import, text-entry submit, audio import, settings access, and the text paste box. No title label; buttons are 12px font pill controls.
- **Main sequence panel** — sentence-sequence list titled "Script and Sentences" for imported/generated practice content; this remains the primary work surface.
- **Bottom control panel** — combines recording controls, RMS feedback, user-recording playback, sentence navigation, loop/delay, speed, and shadowing mode controls.

Style:
- Light tactile/neumorphic surface with off-white background, subtle borders, rounded panels, pill buttons, compact status badges, and visible focus rings.
- Real panel shadow is implemented with `QGraphicsDropShadowEffect` because Qt QSS does not provide true drop shadows.
- QSS is intentionally isolated from C++:
  - `src/app/resources/qss/base/tokens.qss` — app-wide typography, base colors, menu styling.
  - `src/app/resources/qss/components/controls.qss` — shared buttons, inputs, combo boxes, spin boxes, scroll bars.
  - `src/app/resources/qss/components/playback.qss` — playback, record button, visualizer, sliders.
  - `src/app/resources/qss/screens/main_window.qss` — main-window panels, sentence list, status badge.

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
- **Batch updates** — `setValueSilent(key, value)` writes to the in-memory config without saving or emitting. Callers must invoke `endBatchUpdate()` after all silent sets, which performs a single `save()` and emits `configChanged("settings/batch")`. This prevents signal cascades (e.g., 11 individual `setValue` calls in `Q_SettingDialog::saveSettings` would otherwise trigger 6 `recreateTTSEngine` calls).
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
| Whisper | `whisper/model_language` | `en` | Whisper language tag (`en`, `sv`, `zh`, `ja`, `ko`) |
| Playback | `playback/rate` | `1.0` | Playback speed (0.5–2.0) |
| Playback | `playback/mode` | `normal` | Shadowing mode (`normal`, `pause-and-repeat`, `simultaneous`) |
| Log | `log/level` | `debug` | console minimum (file sink is always `debug`) |
| Log | `log/pattern` | `[%Y-%m-%d %H:%M:%S.%e][%t][%^%l%$] %s:%# %! %v` | spdlog pattern |
| Log | `log/to_console`, `log/to_file` | `true`, `true` | sink toggles |

### 5.3 Live-Reload Behavior

`Q_AppMainWindow::onConfigChanged` switches on the changed key:

- `settings/batch` (from settings dialog) → single `recreateTTSEngine()` + `reloadWhisperConfig()`. All settings dialog changes are batched via `setValueSilent` / `endBatchUpdate` to avoid cascading recreations (previously, 11 individual `setValue` calls triggered 6 sequential engine recreations, causing segfaults when async TTS operations were in flight).
- `tts/*` (engine, voice, language, encoding, piper model path) → recreate `ITTSEngine`, recreate `TextContentProvider`, replace content provider on `Q_SessionPlaybackController`, rebind playback connections, restore the active session. Used only for fine-grained control changes (e.g., playback rate slider).
- `whisper/model_path`, `whisper/model_language` → `Q_ASRController::reloadModel(...)` which queues `Q_WhisperWorker::reloadModel` on the worker thread (frees `whisper_context*`; next `doTranscribe` reloads).
- `network/proxy` → re-export proxy env vars in-process.
- `playback/rate` → applies `Q_AudioPlayer::setRate()` immediately.
- `playback/mode` → stored in config; applied on next `restorePlaybackSettings()` / mode change via UI.

---

## 6. Build System

### 6.1 Build Targets (CMakeLists.txt)

| Target | Type | Defined In | Description |
|--------|------|------------|-------------|
| `ThothCore` | Static library | `src/core/CMakeLists.txt` | All business logic. `GLOB_RECURSE` over `src/core/*.cpp` (re-run CMake when adding files). |
| `ThothApp` | Executable | `src/app/CMakeLists.txt` | GUI application. On macOS, a post-build step wraps the debug binary in a minimal `.app` bundle (`Thoth.app/Contents/{MacOS,Resources}` with Info.plist) for proper TSM/IME registration. The `--preset package` preset enables a full macOS `.app` bundle for distribution. |
| `ThothTests` | Executable | `test/CMakeLists.txt` | GoogleTest binary; `gtest_discover_tests` registers each TEST with CTest (timeout 30 s) |
| `RunThothTest` | Custom | `test/CMakeLists.txt` | `ctest --output-on-failure` |
| `ThothDocs` | Custom | root | Doxygen target (working dir `docs/`) |
| `ThothFormat` | Custom | root | clang-format over `src/**/*.{h,cpp}` and `include/**/*.h` |
| `ThothClean` | Custom | root | `cmake -E remove_directory ${CMAKE_BINARY_DIR}` |

A developer `Makefile` at the project root wraps CMake presets: `make build`, `make test`, `make run` (uses `open Thoth.app` on macOS), `make lint`, `make format`, `make clean`, `make package`, `make setup`.

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
| FR-001 | Text-box input | ✅ `QPlainTextEdit` + `loadFromText()` in-memory ingest |
| FR-002 | TXT file import | ✅ |
| FR-005 | Multi-language (en, sv, zh, ja, ko) | ✅ Config + UI + language→voice mapping |
| FR-008 | Text segmentation | ✅ `RegexSegmenter` (`.?!\n` covers en/sv) |
| FR-012 | Google TTS | ✅ via google-cloud-cpp |
| FR-013 | Local + online TTS | ⚠️ GCP works; Piper adapter present but synthesis stub |
| FR-014 | Sentence-level TTS | ✅ |
| FR-015 | TTS caching | ✅ MD5-hashed file cache |
| FR-016 | TTS provider config | ✅ JSON config + live reload |
| FR-022 | Full playback (sentence-by-sentence) | ✅ |
| FR-023 | Single-sentence playback | ✅ |
| FR-024 | Interval playback | ✅ Configurable inter-loop delay |
| FR-025 | Playback controls + speed | ✅ Play/Pause/Prev/Next/Loop/Delay + speed slider 0.5x–2.0x |
| FR-027 | User recording | ✅ via `QAudioSource` |
| FR-028 | Sentence-level recording | ✅ |
| FR-029 | Recording storage | ✅ WAV files in `<cacheDir>/record/` |
| FR-030 | Manual stop | ✅ |
| FR-031 | Pause-and-repeat mode | ✅ Mode combo + `repeatRequested` signal + auto-record trigger |
| FR-032 | Simultaneous shadowing mode | ✅ Recording allowed while playback active |
| FR-033 | Pronunciation scoring | ✅ WER-based (0–100%); `scoreDetail()` returns per-token labels |
| FR-034 | Whisper.cpp integration | ✅ Local STT for scoring path |
| FR-036 | Word-level difference highlighting | ✅ `HtmlDelegate` renders colored tokens (Correct/Missing/Different) + extra tokens |
| FR-003 | Audio file import (WAV) | ✅ `AudioContentProvider` + `onImportFile` routing; WAV decode via existing `WAV::decode` |
| FR-009 | Audio transcription | ✅ `Q_WhisperWorker::doTranscribeFile` path with timestamp extraction |
| FR-017 | Whisper.cpp as STT for source audio | ✅ `transcribeFile` path alongside scoring path |
| FR-018 | Whisper.cpp integration (dual path) | ✅ Scoring + source-audio transcription paths |
| FR-020 | Timestamp extraction | ✅ `wparams.print_timestamps = true` in source-audio path |
| FR-021 | Audio-text alignment | ✅ Whisper segments merged/split into `Sentence` with `audioRange`; range playback via `Q_AudioPlayer::play(path, startMs, endMs)` |
| — | Transcription progress reporting | ✅ `whisper_progress_callback` trampoline → `progressChanged(int)` signal chain through `Q_WhisperWorker` → `Q_ASRController` → status bar |
| — | Concurrency import UX | ✅ Load buttons (LOAD FILE/TEXT TYPE/LOAD AUDIO) disabled during import; re-enabled on session ready or ASR error; `AudioContentProvider` callback not silently dropped on double-click |
| — | Language auto-detection | ✅ `language == "auto"` passes `nullptr` to `whisper_full_params.language` for Whisper's built-in detection |
| — | Sentence-ending punctuation | ✅ Extended `kEnders` to include `…` and `;` for podcast/lecture prose |
| NFR-006 | Async / non-blocking GUI | ✅ QtConcurrent for TTS, `QThread` workers for IO and Whisper |
| NFR-007 | Error handling | ✅ Exceptions + Qt error signals + `QMessageBox` for playback errors |
| NFR-010 | Modular architecture | ✅ 15 public headers, internal/services/controllers/utils split |
| NFR-011 | Provider abstraction | ✅ `ITTSEngine`, `ISentenceScorer`, `IContentProvider`, `IAudioCaptureService`, `IStreamStorage` |
| NFR-012 | Config-based design | ✅ JSON `ConfigStore` |
| NFR-013 | Project structure | ✅ separate `src/core`, `src/app`, `test`, `docs`, `models`, `third_party`, `cmake` |
| NFR-014 | Unit tests | ✅ 7 active test files (27 tests) |
| NFR-016 | API key handling | ✅ env var + auth chain + config-stored path |
| NFR-017 | Local data privacy | ✅ recordings, transcripts, sessions are all local |

### 8.2 In Progress / Partial

| PRD | Feature | Status / Note |
|-----|---------|--------------|
| FR-003 | Audio file import (non-WAV) | ✅ WAV works; MP3/M4A/FLAC via `QAudioDecoder`-based `decodeToMono16kFloat()` |
| FR-007 | Language detection | ⚠️ `"auto"` option passes `nullptr` to Whisper for built-in detection; no UI indicator of detected language yet |
| FR-038 | Session summary | ❌ |
| NFR-009 | Session persistence across restarts | ❌ Sessions live in memory only |
| Config | Config change batching | ✅ `ConfigStore::setValueSilent` + `endBatchUpdate` — prevents signal cascades and segfaults during settings save |
| UI | Settings voice combo safety | ✅ Language change immediately clears voice combo, shows "Loading...", disables until async GCP fetch completes. Falls back to `DefaultVoiceForLanguage()` on premature save |
| UI | File menu simplification | ✅ Removed "Import File" from File menu; import is via INPUT panel buttons only |
| macOS | Debug .app bundle | ✅ Post-build step wraps debug binary in minimal `Thoth.app` bundle for macOS TSM/IME registration. `Makefile run` target uses `open Thoth.app` on macOS |
| Audio | Cache encoding-aware extension | ✅ `AudioCache::_extensionForEncoding()` extracts the encoding from `cacheIdentity` (e.g., `WAV` → `.wav`, `OGG_OPUS` → `.ogg`, `MP3` → `.mp3`). Cached files now have correct extensions matching the configured `audio_encoding`. |

### 8.3 Not Started

| Feature | Priority |
|---------|----------|
| Session persistence / recovery | Medium (NFR-009) |
| Session summary view | Medium (FR-038) |
| Piper synthesis implementation | Medium |
| Language detection (FR-007) | Medium |
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

1. **Single Whisper context** — model switch requires `reloadModel()` (free + lazy reinit on next transcribe).
2. **No session persistence** — close = lose. Recordings remain on disk under `<cacheDir>/record/` but aren't reattached.
3. **Segmenter is English/Swedish-tuned** — `RegexSegmenter` splits on `.?!\n`; CJK punctuation is not handled.
4. **No JP/KR token-level scoring** — WER tokenizes on whitespace, which is wrong for Japanese/Korean. (Low priority per current scope.)
5. **Piper not functional** — `synthesize()` returns empty even when `isAvailable()` is true.
6. **GCP is the de-facto TTS** — without a working Piper, network is required for new sentences (cached audio still plays offline).
7. **No silence-detection auto-stop** — recording runs until the user clicks Stop. (Low priority per current scope.)

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

- `.clang-format` (Google-based, 100 col, 4-space indent)
- `.clang-tidy` (C++20/Qt6-tuned, bugprone/performance/readability/modernize/cppcoreguidelines checks — includes `cppcoreguidelines-use-default-member-init`, `readability-braces-around-statements`, and 40+ other checks)
- `.cmake-format.yaml`
- `.pre-commit-config.yaml` runs clang-format + cmake-format + shellcheck on every commit; clang-tidy + cppcheck + cpplint available as manual-stage hooks
- `CPPLINT.cfg` for Google-style cpplint (manual use)
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
├── include/thoth/                        # Public API (17 headers)
│   ├── AudioContentProvider.h
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
│   ├── Q_GCPVoiceLoader.h
│   └── AuthProviderFactory.h
├── src/
│   ├── app/
│   │   ├── main.cpp
│   │   ├── AppRunner.{h,cpp}
│   │   ├── resources/
│   │   │   ├── resources.qrc
│   │   │   └── qss/
│   │   │       ├── base/        # App-wide tokens and root widget styling
│   │   │       ├── components/  # Shared control/playback styles
│   │   │       └── screens/     # Main-window screen-level styles
│   │   └── ui/
│   │       ├── Q_AppMainWindow.{h,cpp}
│   │       ├── Q_SettingDialog.{h,cpp}
│   │       ├── Q_ShadowingBar.{h,cpp}
│   │       ├── Q_PlaybackControlBar.{h,cpp}
│   │       ├── HtmlDelegate.h
│   │       └── StyleLoader.h
│   └── core/
│       ├── common/         # ConfigStore, Logger, Auth, LockFreeRingBuffer
│       ├── controllers/    # Q_ASRController, Q_RecordController,
│       │                   # Q_SessionPlaybackController, Q_WhisperWorker
│       ├── entities/       # WAV
│       ├── internal/       # 13 internal headers
│       ├── services/       # TTS, downloader, audio capture, storage,
│       │                   # content provider (Text + Audio), audio cache, audio player
│       └── utils/          # Segmenters, TextParser, FileExtractors,
│                           # WERScorer, AudioTools, Timer
├── test/                   # 7 GoogleTest files + test_main.cpp (27 tests)
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
├── .clang-tidy
├── .cmake-format.yaml
├── .pre-commit-config.yaml
├── CPPLINT.cfg
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

### Phase 1 — MVP Completion ✅ COMPLETE

- [x] Text-box input (FR-001)
- [x] Swedish language support (config + segmentation rules)
- [x] Word-level diff highlighting in sentence list (correct / missing / different)
- [x] Pause-and-repeat mode (auto-pause after reference; user records)
- [x] Simultaneous shadowing mode (record while playback active)
- [x] Playback speed control (0.5x–2.0x slider)
- [x] Settings dialog: Whisper language combo + playback speed + mode persistence

### Phase 2 — Audio Import ✅ Mostly complete (WAV done; non-WAV + language detection pending)

- [x] Audio file import — `.wav` via `WAV::decode` + `AudioContentProvider`
- [x] Source-audio Whisper transcription with timestamp extraction
- [x] Audio-text alignment (Whisper segments → `Sentence::audioRange`)
- [x] Range playback (`Q_AudioPlayer::play(path, startMs, endMs)`)
- [x] WAV resampling (linear interpolation in `WAV::resample`)
- [x] GCP TTS multi-language: async voice discovery (`Q_GCPVoiceLoader`), 4-language combo, voice auto-mapping, encoding selector, engine-aware UI toggle
- [x] Audio cache refactored to engine-agnostic `cacheIdentity` (no longer hard-wired to GoogleTTSConfig)
- [ ] Non-WAV decode (MP3/M4A/FLAC via `QAudioDecoder`)
- [ ] Language detection (FR-007)

### Phase 3 — Practice Modes & Feedback

- [ ] Session summary panel (avg score, weak words, duration)
- [ ] Session persistence (JSON serialization or SQLite)

### Phase 4 — Polish

- [ ] Piper TTS full implementation + voice model browser
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

# Or use the developer Makefile
make build          # Debug build
make run            # Build + launch (uses open Thoth.app on macOS)
make test           # Build + run all tests
make lint           # Run pre-commit hooks
make format         # clang-format all sources
make clean          # Remove debug build artifacts
make clean-all      # Remove all build dirs and dist/
make package        # Create macOS .dmg distribution

# Run tests directly
cd build/debug && ctest --output-on-failure
cmake --build build/debug --target RunThothTest

# Generate documentation
cmake --build build/debug --target ThothDocs

# Re-symlink compile_commands.json for clangd (after configure)
ln -sf debug/compile_commands.json build/compile_commands.json
```
