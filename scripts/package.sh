#!/usr/bin/env bash
# scripts/package.sh — Build Thoth.app and create a distributable macOS DMG.
#
# Usage:
#   ./scripts/package.sh [options]
#
# Options:
#   --version X.Y.Z    Override version string (default: content of VERSION file)
#   --skip-build       Skip cmake configure+build; use existing build/package output
#   --help             Show this help and exit
#
# Output:
#   dist/Thoth-<version>-macOS-arm64.dmg
#
# Prerequisites:
#   - Xcode Command Line Tools (xcode-select --install)
#   - Ninja          (brew install ninja)
#   - Qt6            (brew install qtbase qtmultimedia)
#   - vcpkg          ($VCPKG_ROOT or ~/vcpkg)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# ── Parse arguments ───────────────────────────────────────────────────────────
VERSION=$(cat "${REPO_ROOT}/VERSION" 2>/dev/null || echo "0.0.0")
SKIP_BUILD=false

while [[ $# -gt 0 ]]; do
    case "$1" in
        --version)    VERSION="$2"; shift 2 ;;
        --skip-build) SKIP_BUILD=true; shift ;;
        --help)
            sed -n '2,20p' "$0" | grep '^#' | sed 's/^#[[:space:]]*//'
            exit 0
            ;;
        *) echo "Unknown option: $1  (try --help)"; exit 1 ;;
    esac
done

BUILD_DIR="${REPO_ROOT}/build/package"
DIST_DIR="${REPO_ROOT}/dist"
APP_BUNDLE="Thoth.app"
DMG_NAME="Thoth-${VERSION}-macOS-arm64.dmg"

# ── Check prerequisites ───────────────────────────────────────────────────────
echo "==> Checking prerequisites..."

ok()   { echo "  [ok] $*"; }
fail() { echo "  [!!] $*"; exit 1; }

command -v cmake    &>/dev/null && ok cmake    || fail "cmake not found.    Install: xcode-select --install"
command -v ninja    &>/dev/null && ok ninja    || fail "ninja not found.    Install: brew install ninja"
command -v codesign &>/dev/null && ok codesign || fail "codesign not found. Install: xcode-select --install"
command -v hdiutil  &>/dev/null && ok hdiutil  || fail "hdiutil not found.  Requires macOS."

MACDEPLOYQT=""
for candidate in \
    "$(command -v macdeployqt 2>/dev/null || true)" \
    "/opt/homebrew/bin/macdeployqt" \
    "/usr/local/bin/macdeployqt"; do
    [[ -x "${candidate}" ]] && MACDEPLOYQT="${candidate}" && break
done
[[ -n "${MACDEPLOYQT}" ]] && ok "macdeployqt (${MACDEPLOYQT})" \
    || fail "macdeployqt not found. Install Qt: brew install qtbase qtmultimedia"

VCPKG_ROOT="${VCPKG_ROOT:-${HOME}/vcpkg}"
[[ -d "${VCPKG_ROOT}" ]] && ok "vcpkg (${VCPKG_ROOT})" \
    || fail "VCPKG_ROOT=${VCPKG_ROOT} not found. Run scripts/setup-macos.sh first."

# ── Build ─────────────────────────────────────────────────────────────────────
if [[ "${SKIP_BUILD}" == "false" ]]; then
    echo ""
    echo "==> Configuring (preset: package, version: ${VERSION})..."

    # Resolve Qt prefix for cmake (Homebrew arm64 layout)
    QT_PREFIX=""
    if command -v brew &>/dev/null; then
        QT_BASE=$(brew --prefix qtbase      2>/dev/null || true)
        QT_MM=$(brew  --prefix qtmultimedia 2>/dev/null || true)
        [[ -n "${QT_BASE}" && -n "${QT_MM}" ]] && QT_PREFIX="${QT_BASE}:${QT_MM}"
    fi

    CMAKE_EXTRA_ARGS=(-DTHOTH_VERSION="${VERSION}")
    [[ -n "${QT_PREFIX}" ]] && CMAKE_EXTRA_ARGS+=(-DCMAKE_PREFIX_PATH="${QT_PREFIX}")

    cmake --preset package "${CMAKE_EXTRA_ARGS[@]}"

    echo "==> Building..."
    cmake --build --preset package --parallel
fi

# ── Locate built app bundle ───────────────────────────────────────────────────
BUILT_APP="${BUILD_DIR}/src/app/${APP_BUNDLE}"
if [[ ! -d "${BUILT_APP}" ]]; then
    cat <<ERR
Error: ${BUILT_APP} not found after build.
  - Make sure the build succeeded (run without --skip-build).
  - The 'package' preset sets THOTH_MACOS_BUNDLE=ON which produces a .app bundle.
ERR
    exit 1
fi

# ── Stage ─────────────────────────────────────────────────────────────────────
STAGE_DIR="${BUILD_DIR}/stage"
echo ""
echo "==> Staging ${APP_BUNDLE} -> ${STAGE_DIR}..."
rm -rf "${STAGE_DIR}"
mkdir -p "${STAGE_DIR}"
cp -R "${BUILT_APP}" "${STAGE_DIR}/${APP_BUNDLE}"

# ── Bundle Whisper.cpp models ─────────────────────────────────────────────────
echo "==> Bundling Whisper.cpp models..."
MODELS_SRC="${REPO_ROOT}/models"
MODELS_DST="${STAGE_DIR}/${APP_BUNDLE}/Contents/Resources/models"
if [[ -d "${MODELS_SRC}" ]] && [[ -n "$(ls -A "${MODELS_SRC}" 2>/dev/null)" ]]; then
    mkdir -p "${MODELS_DST}"
    cp -R "${MODELS_SRC}/"* "${MODELS_DST}/"
    echo "  -> Copied models: $(ls "${MODELS_DST}")"
else
    echo "  -> No models found in ${MODELS_SRC}, skipping."
fi

# ── macdeployqt — bundle Qt frameworks ───────────────────────────────────────
echo "==> Running macdeployqt (bundling Qt frameworks + plugins)..."
"${MACDEPLOYQT}" "${STAGE_DIR}/${APP_BUNDLE}" -verbose=1

# ── Ad-hoc code signing ───────────────────────────────────────────────────────
# Ad-hoc signing allows the app to pass Gatekeeper's basic checks on the user's
# machine (right-click → Open on first run). For App Store / notarized release,
# replace '-' with your Developer ID certificate.
echo "==> Ad-hoc signing..."
codesign --deep --force --sign - "${STAGE_DIR}/${APP_BUNDLE}"

# ── Create DMG ────────────────────────────────────────────────────────────────
echo ""
echo "==> Creating DMG..."
mkdir -p "${DIST_DIR}"

DMG_CONTENT=$(mktemp -d)
trap 'rm -rf "${DMG_CONTENT}"' EXIT

cp -R "${STAGE_DIR}/${APP_BUNDLE}" "${DMG_CONTENT}/"
# /Applications symlink gives users the familiar drag-to-install UI
ln -s /Applications "${DMG_CONTENT}/Applications"

hdiutil create \
    -volname "Thoth ${VERSION}" \
    -srcfolder "${DMG_CONTENT}" \
    -ov -format UDZO \
    -fs HFS+ \
    "${DIST_DIR}/${DMG_NAME}"

# ── Summary ───────────────────────────────────────────────────────────────────
DMG_PATH="${DIST_DIR}/${DMG_NAME}"
DMG_SIZE=$(du -sh "${DMG_PATH}" 2>/dev/null | cut -f1 || echo "?")

echo ""
echo "┌─────────────────────────────────────────────────────────────┐"
echo "│  Package complete                                           │"
echo "├─────────────────────────────────────────────────────────────┤"
printf "│  File : %-52s│\n" "${DMG_NAME}"
printf "│  Size : %-52s│\n" "${DMG_SIZE}"
printf "│  Path : %-52s│\n" "${DIST_DIR}/"
echo "├─────────────────────────────────────────────────────────────┤"
echo "│  Install: open the DMG, drag Thoth to Applications.        │"
echo "│  First launch: right-click → Open to bypass Gatekeeper.    │"
echo "│  Microphone: grant access in System Settings → Privacy.    │"
echo "└─────────────────────────────────────────────────────────────┘"
