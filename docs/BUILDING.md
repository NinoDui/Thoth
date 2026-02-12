# Thoth Build Guide

Cross-platform build instructions for Thoth (macOS, Ubuntu, Windows).

## Prerequisites

- CMake 3.20+
- C++20 compatible compiler (Clang, GCC, MSVC)
- Git

## Dependency Management

Thoth uses **vcpkg manifest mode** for C++ dependencies. The following are managed by vcpkg:

| Package | Purpose |
|---------|---------|
| nlohmann-json | JSON parsing |
| spdlog | Logging |
| google-cloud-cpp | GCP Text-to-Speech API |
| gtest | Unit testing |

Qt6 (Core, Concurrent, Widgets, Network, Multimedia, Test) and whisper.cpp are handled separately (see platform sections).

---

## macOS

### Quick Setup (Recommended)

```bash
./scripts/setup-macos.sh
```

Then build (with `VCPKG_ROOT` set, Thoth auto-uses `cmake/ThothVcpkgToolchain.cmake` which forces x64-osx on arm64):

```bash
export VCPKG_ROOT="$HOME/vcpkg"
cmake -B build -DCMAKE_PREFIX_PATH="$(brew --prefix qtbase):$(brew --prefix qtmultimedia)"
cmake --build build
```

### Manual Setup

1. **Xcode Command Line Tools**
   ```bash
   xcode-select --install
   ```

2. **Homebrew** (Qt6: Core, Concurrent, Widgets, Network, Multimedia, Test only)
   ```bash
   brew install qtbase qtmultimedia
   ```

3. **vcpkg**
   ```bash
   git clone https://github.com/Microsoft/vcpkg.git ~/vcpkg
   ~/vcpkg/bootstrap-vcpkg.sh
   ```

4. **Git submodule**
   ```bash
   git submodule update --init --recursive
   ```

### Build

```bash
export VCPKG_ROOT="$HOME/vcpkg"
cmake -B build -DCMAKE_PREFIX_PATH="$(brew --prefix qtbase):$(brew --prefix qtmultimedia)"
cmake --build build
```

**Note (Apple Silicon):** grpc has a known build failure on `arm64-osx` (`_gRPC_CPP_PLUGIN-NOTFOUND`). Thoth uses `cmake/ThothVcpkgToolchain.cmake` which forces `x64-osx` triplet on arm64; the app runs via Rosetta 2.

**If you previously failed with arm64-osx:** You MUST remove old artifacts before reconfiguring:
```bash
rm -rf build vcpkg_installed
rm -rf $VCPKG_ROOT/buildtrees/grpc $VCPKG_ROOT/buildtrees/abseil \
       $VCPKG_ROOT/buildtrees/protobuf $VCPKG_ROOT/buildtrees/re2 \
       $VCPKG_ROOT/buildtrees/upb $VCPKG_ROOT/buildtrees/c-ares
```
The Thoth toolchain forces x64-osx on all macOS builds.

---

## Ubuntu 22.04

### System Packages (Qt6, tools)

```bash
sudo apt update
sudo apt install -y clang lldb clangd
sudo apt install -y qt6-base-dev qt6-multimedia-dev  # Core, Widgets, Network, Concurrent, Multimedia, Test
sudo apt install -y libgl1-mesa-dev libxkbcommon-dev libvulkan-dev
sudo apt install -y gstreamer1.0-libav  # optional, for audio playback
```

### vcpkg

```bash
git clone https://github.com/Microsoft/vcpkg.git ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh
```

### Build

```bash
export VCPKG_ROOT="$HOME/vcpkg"
git submodule update --init --recursive

cmake -B build \
  -DCMAKE_TOOLCHAIN_FILE="${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" \
  -DCMAKE_PREFIX_PATH="/usr"

cmake --build build
```

---

## Windows

### Tools

```powershell
winget install Git.Git
winget install Kitware.CMake
winget install Microsoft.VisualStudio.2022.BuildTools
# Include "Desktop development with C++" workload
```

### Qt6

Install via [Qt Online Installer](https://www.qt.io/download-qt-installer) or [aqt](https://github.com/miurahr/aqtinstall):

```powershell
pip install aqtinstall
aqt install-qt windows desktop 6.6.0 win64_mingw  # or msvc2019_64
```

### vcpkg

```powershell
git clone https://github.com/Microsoft/vcpkg.git %USERPROFILE%\vcpkg
%USERPROFILE%\vcpkg\bootstrap-vcpkg.bat
```

### Build

```powershell
set VCPKG_ROOT=%USERPROFILE%\vcpkg
git submodule update --init --recursive

cmake -B build ^
  -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake ^
  -DCMAKE_PREFIX_PATH="C:\Qt\6.6.0\msvc2019_64"  # adjust to your Qt path

cmake --build build --config Release
```

---

## Targets

| Target | Description |
|--------|-------------|
| `ThothApp` | Main application |
| `ThothTests` | Unit tests |
| `RunThothTest` | Run tests via CTest |
| `ThothFormat` | Format code with clang-format |
| `ThothClean` | Remove build directory |

---

## vcpkg Baseline

To update locked package versions:

```bash
vcpkg x-update-baseline
```

To add an initial baseline to a new manifest:

```bash
vcpkg x-update-baseline --add-initial-baseline
```
