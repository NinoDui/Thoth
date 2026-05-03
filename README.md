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

## Google Cloud TTS Setup (first-time users)

Thoth uses Google Cloud Text-to-Speech for generating reference audio. You need a GCP service account with the Text-to-Speech API enabled.

### 1. Create a Google Cloud project

1. Go to [console.cloud.google.com](https://console.cloud.google.com) and create a new project (or select an existing one).
2. Enable billing if prompted (TTS has [free tier](https://cloud.google.com/text-to-speech/pricing) — 1 million characters/month for standard voices).

### 2. Enable the Text-to-Speech API

Go to **APIs & Services → Library**, search for "Cloud Text-to-Speech API", and click **Enable**.

### 3. Create a service account and download the key

1. Go to **APIs & Services → Credentials**.
2. Click **Create Credentials → Service Account**.
3. Give it a name (e.g. `thoth-tts`) and click **Create and Continue**.
4. For the role, select **Cloud Text-to-Speech User** (or **Basic → Viewer** for minimal permissions).
5. Click **Done**, then click the service account email in the list.
6. Go to the **Keys** tab and click **Add Key → Create New Key → JSON**.
7. Save the downloaded `.json` file to a secure location on your machine.

### 4. Point Thoth to the credential file

- **Via Settings (recommended)**: Open Thoth, go to **File → Settings**, paste the path to the `.json` file in the **Credential** field, and click **OK**.
- **Via environment variable**: `export GOOGLE_APPLICATION_CREDENTIALS=/path/to/your-key.json` before launching Thoth.

On first launch without credentials, Thoth will prompt you to select the credential file automatically.

### 5. Select a voice

Open **File → Settings**, choose your language from the dropdown. Thoth will automatically load available voices from Google Cloud for that language. Pick a voice from the list and click **OK**.

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
