#!/usr/bin/env bash
# Thoth Build Script (Cross-Platform via CMake Presets)
# Usage: ./scripts/build.sh [--release] [--clean]

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# Parse arguments
BUILD_TYPE="debug"
CLEAN_BUILD=false

for arg in "$@"; do
    case $arg in
        --release)
            BUILD_TYPE="release"
            ;;
        --clean)
            CLEAN_BUILD=true
            ;;
        --help)
            echo "Usage: $0 [--release] [--clean]"
            exit 0
            ;;
    esac
done

echo "==> Building Thoth (${BUILD_TYPE})..."

# Check for VCPKG_ROOT
if [ -z "${VCPKG_ROOT:-}" ]; then
    # Try default location
    if [ -d "$HOME/vcpkg" ]; then
        export VCPKG_ROOT="$HOME/vcpkg"
    else
        echo "Error: VCPKG_ROOT is not set and $HOME/vcpkg not found."
        echo "Please install vcpkg or set VCPKG_ROOT."
        exit 1
    fi
fi

# Platform-specific setup
EXTRA_CMAKE_ARGS=""
if [[ "$(uname)" == "Darwin" ]]; then
    # macOS: specific setup for Qt path if using Homebrew
    if command -v brew &>/dev/null; then
        QT_BASE=$(brew --prefix qtbase 2>/dev/null || true)
        QT_MM=$(brew --prefix qtmultimedia 2>/dev/null || true)
        if [ -n "$QT_BASE" ] && [ -n "$QT_MM" ]; then
            export CMAKE_PREFIX_PATH="$QT_BASE:$QT_MM"
            echo "  -> Found Qt6 at: $CMAKE_PREFIX_PATH"
        fi
        EXTRA_CMAKE_ARGS="$EXTRA_CMAKE_ARGS -DQT_ADDITIONAL_PACKAGES_PREFIX_PATH=$(brew --prefix qtmultimedia)"
    fi
fi

# Locate Ninja
NINJA_PATH=$(command -v ninja || true)
if [ -n "$NINJA_PATH" ]; then
    echo "  -> Found Ninja at: $NINJA_PATH"
    EXTRA_CMAKE_ARGS="$EXTRA_CMAKE_ARGS -DCMAKE_MAKE_PROGRAM=$NINJA_PATH"
else
    echo "  -> Ninja not found in PATH, relying on CMake..."
fi

# Clean if requested
if [ "$CLEAN_BUILD" = true ]; then
    echo "==> Cleaning build directory..."
    rm -rf "${PROJECT_ROOT}/build/${BUILD_TYPE}"
fi

# Configure and Build
cd "${PROJECT_ROOT}"

echo "==> Configuring (Preset: ${BUILD_TYPE})..."
if [ -n "$EXTRA_CMAKE_ARGS" ]; then
    echo "  -> Extra Args: $EXTRA_CMAKE_ARGS"
fi
cmake --preset "${BUILD_TYPE}" $EXTRA_CMAKE_ARGS

echo "==> Building (Preset: ${BUILD_TYPE})..."
cmake --build --preset "${BUILD_TYPE}"

echo "==> Build complete!"
echo "    Artifacts in: build/${BUILD_TYPE}"
