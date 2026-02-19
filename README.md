# Thoth
An echo software for language learning, implemented by C++.

## Development Prep

See [docs/BUILDING.md](docs/BUILDING.md) for full cross-platform build instructions.

### macOS

```shell
./scripts/setup-macos.sh
# Then: export VCPKG_ROOT=$HOME/vcpkg
#       cmake -B build -DCMAKE_PREFIX_PATH="$(brew --prefix qtbase):$(brew --prefix qtmultimedia)"
#       cmake --build build
# If previous arm64-osx build failed: rm -rf build vcpkg_installed + vcpkg/buildtrees/grpc etc.
```

### Ubuntu 22.04

```shell
sudo apt update
sudo apt install clang lldb clangd valgrind
sudo apt install qt6-base-dev qt6-multimedia-dev  # Core, Widgets, Network, Concurrent, Multimedia, Test
sudo apt install libgl1-mesa-dev libxkbcommon-dev libvulkan-dev
sudo apt install gstreamer1.0-libav  # optional, for audio playback

# vcpkg (installs nlohmann-json, spdlog, google-cloud-cpp, gtest from vcpkg.json)
git clone https://github.com/Microsoft/vcpkg.git ~/vcpkg && ~/vcpkg/bootstrap-vcpkg.sh
git submodule update --init --recursive

cmake -B build -DCMAKE_TOOLCHAIN_FILE=$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

### Windows

*Have to say I hate setting up the dev environment on Windows.*

```shell
winget install Git.Git
winget install Kitware.CMake
winget install Microsoft.VisualStudio.2022.BuildTools
```

## Next TODOs
- [] Audio Recorder for shadowing
- [] ASR / STT (integration of OpenAI Whisper), WER / Levenshtein Distance
- [] Pitch & Prosody Analysis (e.g. Aubio), for F0/Pitch or Energy curves
