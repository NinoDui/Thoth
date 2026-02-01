# Thoth
An echo software for language learning, implemented by C++.

## Development Prep

### Ubuntu 22.04

```shell
sudo apt update
sudo apt install clang lldb
sudo apt install valgrind
sudo apt install qt6-base-dev qt6-multimedia-dev libqt6core5compat6-dev
sudo apt install clangd
sudo apt install libgl1-mesa-dev
sudo apt install libxkbcommon-dev libvulkan-devsudo apt install
sudo apt install nlohmann-json3-dev
sudo apt install gstreamer1.0-libav # optional, I am not playing the audio on Ubuntu

# it's highly suggested to install google-cloud-cpp via vcpkg!!!
vcpkg install google-cloud-cpp[core,texttospeech]
vcpkg install spdlog
```

### Windows

*Have to say I hate setting up the dev environment on Windows.*

```shell
winget install Git.Git
winget install Kitware.CMake
winget install Microsoft.VisualStudio.2022.BuildTools
```

## Next TODOs
[] Audio Recorder for shadowing
[] ASR / STT (integration of OpenAI Whisper), WER / Levenshtein Distance
[] Pitch & Prosody Analysis (e.g. Aubio), for F0/Pitch or Energy curves
