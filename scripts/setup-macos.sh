#!/usr/bin/env bash
# Thoth macOS Development Environment Setup
# Installs Xcode CLT, Homebrew, Qt6, vcpkg, and project dependencies.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck disable=SC2034  # used by commented-out vcpkg install block below
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

# 3. Qt6 (qtbase + qtmultimedia) and Ninja build system
echo "==> Installing Qt6 and Ninja..."
brew install qtbase qtmultimedia ninja

# 4. vcpkg
VCPKG_ROOT="${VCPKG_ROOT:-$HOME/vcpkg}"
echo "==> Setting up vcpkg at ${VCPKG_ROOT}..."
if [ ! -d "$VCPKG_ROOT" ]; then
    git clone https://github.com/Microsoft/vcpkg.git "$VCPKG_ROOT"
    "$VCPKG_ROOT/bootstrap-vcpkg.sh"
fi
export VCPKG_ROOT

for pkg in abseil protobuf grpc re2 c-ares; do
    if brew list "$pkg" &>/dev/null 2>&1; then
        echo "  -> Unlinking $pkg (conflicts with vcpkg x64-osx build)"
        brew unlink "$pkg" 2>/dev/null || true
    fi
done

# 5. Determine triplet (Apple Silicon uses x64-osx workaround: grpc fails on arm64-osx)
ARCH=$(uname -m)
if [ "$ARCH" = "arm64" ]; then
    TRIPLET="x64-osx"   # grpc _gRPC_CPP_PLUGIN-NOTFOUND on arm64-osx; x64 runs via Rosetta 2
else
    TRIPLET="x64-osx"
fi
echo "==> Using vcpkg triplet: ${TRIPLET}"

# 6. Install vcpkg dependencies (optional pre-install, CMake will do it too)
# We skip explicit install here because CMakePresets.json handles it during configure
# efficiently in the project root vcpkg_installed directory.
# echo "==> Installing vcpkg dependencies..."
# cd "$PROJECT_ROOT"
# "$VCPKG_ROOT/vcpkg" install --triplet "$TRIPLET"

# 7. whisper.cpp submodule
echo "==> Initializing git submodules..."
git submodule update --init --recursive

echo ""
echo "=== Setup complete ==="
echo ""
echo "To build (Debug default):"
echo "  ./scripts/build.sh"
echo ""
echo "To build Release:"
echo "  ./scripts/build.sh --release"
echo ""
