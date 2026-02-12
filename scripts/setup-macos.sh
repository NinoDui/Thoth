#!/usr/bin/env bash
# Thoth macOS Development Environment Setup
# Installs Xcode CLT, Homebrew, Qt6, vcpkg, and project dependencies.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# 1. Check Xcode Command Line Tools
echo "==> Checking Xcode Command Line Tools..."
if ! xcode-select -p &>/dev/null; then
    echo "Xcode Command Line Tools not found. Installing..."
    xcode-select --install
    echo "Please complete the Xcode installation in the popup, then re-run this script."
    exit 1
fi

# 2. Homebrew
echo "==> Checking Homebrew..."
if ! command -v brew &>/dev/null; then
    echo "Homebrew not found. Installing..."
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    [[ -f /opt/homebrew/bin/brew ]] && eval "$(/opt/homebrew/bin/brew shellenv)"
fi

# 3. Qt6 components only (Core, Concurrent, Widgets, Network, Multimedia, Test)
echo "==> Installing Qt6 (qtbase + qtmultimedia)..."
brew install qtbase qtmultimedia

# 4. vcpkg
VCPKG_ROOT="${VCPKG_ROOT:-$HOME/vcpkg}"
echo "==> Setting up vcpkg at ${VCPKG_ROOT}..."
if [ ! -d "$VCPKG_ROOT" ]; then
    git clone https://github.com/Microsoft/vcpkg.git "$VCPKG_ROOT"
    "$VCPKG_ROOT/bootstrap-vcpkg.sh"
fi
export VCPKG_ROOT

# 5. Determine triplet (Apple Silicon uses x64-osx workaround: grpc fails on arm64-osx)
ARCH=$(uname -m)
if [ "$ARCH" = "arm64" ]; then
    TRIPLET="x64-osx"   # grpc _gRPC_CPP_PLUGIN-NOTFOUND on arm64-osx; x64 runs via Rosetta 2
    # Clean stale arm64-osx artifacts if switching from failed arm64 build
    if [ -d "$PROJECT_ROOT/vcpkg_installed/arm64-osx" ] || [ -d "$PROJECT_ROOT/build/vcpkg_installed/arm64-osx" ]; then
        echo "==> Removing stale arm64-osx artifacts (project + vcpkg buildtrees)..."
        rm -rf "$PROJECT_ROOT/build" "$PROJECT_ROOT/vcpkg_installed"
        rm -rf "$VCPKG_ROOT/buildtrees/grpc" "$VCPKG_ROOT/buildtrees/abseil" \
               "$VCPKG_ROOT/buildtrees/protobuf" "$VCPKG_ROOT/buildtrees/re2" \
               "$VCPKG_ROOT/buildtrees/upb" "$VCPKG_ROOT/buildtrees/c-ares" 2>/dev/null || true
    fi
else
    TRIPLET="x64-osx"
fi
echo "==> Using vcpkg triplet: ${TRIPLET}"

# 6. Install vcpkg dependencies from manifest
echo "==> Installing vcpkg dependencies (this may take 15-30 min for google-cloud-cpp)..."
cd "$PROJECT_ROOT"
"$VCPKG_ROOT/vcpkg" install --triplet "$TRIPLET"

# 7. whisper.cpp submodule
echo "==> Initializing git submodules..."
git submodule update --init --recursive

echo ""
echo "=== Setup complete ==="
echo ""
echo "Build with (VCPKG_ROOT must be set):"
echo "  export VCPKG_ROOT=\"${VCPKG_ROOT}\""
echo "  rm -rf build vcpkg_installed"
echo "  rm -rf \$VCPKG_ROOT/buildtrees/grpc \$VCPKG_ROOT/buildtrees/abseil   # if switching from arm64-osx"
echo "  cmake -B build -DCMAKE_PREFIX_PATH=\"\$(brew --prefix qtbase):\$(brew --prefix qtmultimedia)\""
echo "  cmake --build build"
echo ""
