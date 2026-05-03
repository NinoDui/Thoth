# Thoth

Cross-platform (macOS / Windows) desktop app for foreign-language shadowing practice.
Implemented in **C++20** with **Qt 6**.

## Features

- **Text input**: paste text or import `.txt` files → auto-segmented into sentences → TTS reference audio
- **Audio input**: import `.wav` files → Whisper transcription with timestamps → source-audio range playback (no TTS needed)
- **Recording**: capture your shadowing attempt with the built-in microphone
- **Scoring**: local whisper.cpp transcription + WER-based token-level scoring with word-colored diff
- **Shadowing modes**: Normal, Pause-and-Repeat (auto-records after playback), Simultaneous (record while playing)
- **Playback speed**: adjustable 0.5x–2.0x
- **Multi-language**: English, Swedish, Chinese, Japanese, Korean (TTS + STT language config)
- **Live-reload config**: change TTS engine, voice, Whisper model, or proxy without restart
- **Local-first**: recordings and transcripts never leave your machine

## Quick Start (macOS)

```shell
./scripts/setup-macos.sh
export VCPKG_ROOT=$HOME/vcpkg

cmake --preset debug
cmake --build build/debug
./build/debug/src/app/ThothApp
```

## Build (cross-platform)

| Platform | Instructions |
|----------|-------------|
| macOS | `./scripts/setup-macos.sh` → `cmake --preset debug` → `cmake --build build/debug` |
| Ubuntu 22.04 | `sudo apt install qt6-base-dev qt6-multimedia-dev` → vcpkg → `cmake --preset debug` |
| Windows | `winget install` Qt + CMake + VS2022 → vcpkg → `cmake --preset debug` |

See [CMakePresets.json](CMakePresets.json) for available presets (`debug`, `release`).

## Develop

```shell
# Run tests (27 tests)
cmake --build build/debug --target ThothTests
cd build/debug && ctest --output-on-failure

# Code quality (pre-commit hooks)
pre-commit install                    # auto: clang-format, cmake-format, shellcheck
pre-commit run --all-files            # run all auto hooks
pre-commit run --hook-stage manual clang-tidy  # manual: clang-tidy, cppcheck, cpplint
```

## Architecture

```
ThothApp (Qt Widgets GUI)
    └── ThothCore (static library)
          ├── Public API: include/thoth/ (16 headers)
          ├── Controllers: Q_SessionPlaybackController, Q_RecordController, Q_ASRController
          ├── Services: TTS (GCP/Piper), STT (whisper.cpp), AudioCapture, AudioStorage
          └── Content Providers: TextContentProvider, AudioContentProvider
```

## Docs

| Document | Purpose |
|----------|---------|
| [docs/PRD.md](docs/PRD.md) | Product requirements |
| [docs/PDD.md](docs/PDD.md) | Project development doc (architecture, status, roadmap) |
| [docs/TASKS.md](docs/TASKS.md) | Task list by priority |
| [docs/BUILDING.md](docs/BUILDING.md) | Detailed cross-platform build guide |

## Dependencies

| Library | Purpose |
|---------|---------|
| Qt 6 | GUI, audio, networking, threads (Core, Widgets, Multimedia, Network, Concurrent, Test) |
| whisper.cpp | Local STT (git submodule) |
| Google Cloud TTS C++ SDK | Online TTS |
| spdlog | Logging |
| nlohmann/json | Configuration |
| GoogleTest + GMock | Testing |
| vcpkg | Package manager (manifest mode) |
