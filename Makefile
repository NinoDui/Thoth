# ──────────────────────────────────────────────────────────────────────────────
# Thoth — Developer Makefile
#
# Wraps CMake presets and scripts/ for common workflows.
# Run  make help  (default) for a list of all targets.
#
# Variables you can override on the command line:
#   VCPKG_ROOT=/path/to/vcpkg   (default: ~/vcpkg)
#   VERSION=1.2.3               (default: content of VERSION file)
# ──────────────────────────────────────────────────────────────────────────────

SHELL := /usr/bin/env bash

# ── Project variables ─────────────────────────────────────────────────────────
VERSION     ?= $(shell cat VERSION 2>/dev/null | tr -d '[:space:]' || echo "0.0.0")
VCPKG_ROOT  ?= $(HOME)/vcpkg

BUILD_DEBUG   := build/debug
BUILD_RELEASE := build/release
APP_BIN       := $(BUILD_DEBUG)/src/app/ThothApp

# Qt6 Homebrew prefix — evaluated once at parse time, not per recipe
_QT_BASE := $(shell brew --prefix qtbase      2>/dev/null || true)
_QT_MM   := $(shell brew --prefix qtmultimedia 2>/dev/null || true)
ifneq ($(_QT_BASE),)
  _QT_PREFIX := $(_QT_BASE):$(_QT_MM)
  _CMAKE_QT  := CMAKE_PREFIX_PATH="$(_QT_PREFIX)"
endif

# cmake env prefix — set for every cmake invocation that does not go through build.sh
_CMAKE_ENV := VCPKG_ROOT="$(VCPKG_ROOT)" $(_CMAKE_QT)

# ── Default target ────────────────────────────────────────────────────────────
.DEFAULT_GOAL := help

.PHONY: help setup config config-release build build-release \
        test run lint format docs package clean clean-all

# ── Help ──────────────────────────────────────────────────────────────────────
help:
	@printf '\n\033[1mThoth v$(VERSION)\033[0m  —  Developer Makefile\n\n'
	@printf '\033[4mDevelopment\033[0m\n'
	@printf '  %-28s %s\n' 'make build'               'Debug build  (default)'
	@printf '  %-28s %s\n' 'make build-release'       'Optimised release build'
	@printf '  %-28s %s\n' 'make config'              'CMake configure only (debug)'
	@printf '  %-28s %s\n' 'make config-release'      'CMake configure only (release)'
	@printf '  %-28s %s\n' 'make test'                'Build + run all tests'
	@printf '  %-28s %s\n' 'make run'                 'Build debug + launch ThothApp'
	@printf '  %-28s %s\n' 'make lint'                'Run pre-commit hooks on all files'
	@printf '  %-28s %s\n' 'make format'              'clang-format all sources'
	@printf '  %-28s %s\n' 'make docs'                'Generate Doxygen documentation'
	@printf '\n\033[4mMaintenance\033[0m\n'
	@printf '  %-28s %s\n' 'make clean'               'Remove debug build artifacts'
	@printf '  %-28s %s\n' 'make clean-all'           'Remove all build dirs and dist/'
	@printf '\n\033[4mDistribution  (macOS)\033[0m\n'
	@printf '  %-28s %s\n' 'make package'             'Build + create  dist/*.dmg'
	@printf '  %-28s %s\n' 'make package VERSION=x.y.z' 'Package with explicit version'
	@printf '\n\033[4mFirst-time setup\033[0m\n'
	@printf '  %-28s %s\n' 'make setup'               'Install deps: Homebrew / Qt6 / vcpkg'
	@printf '\n'
	@printf 'Active variables:\n'
	@printf '  %-16s = %s\n' 'VCPKG_ROOT' '$(VCPKG_ROOT)'
	@printf '  %-16s = %s\n' 'VERSION'    '$(VERSION)'
	@printf '\n'

# ── First-time setup ──────────────────────────────────────────────────────────
setup:
	./scripts/setup-macos.sh

# ── Configure only  (no build) ────────────────────────────────────────────────
config:
	$(_CMAKE_ENV) cmake --preset debug

config-release:
	$(_CMAKE_ENV) cmake --preset release

# ── Builds — delegate to build.sh which handles Qt path detection ─────────────
build:
	VCPKG_ROOT="$(VCPKG_ROOT)" ./scripts/build.sh

build-release:
	VCPKG_ROOT="$(VCPKG_ROOT)" ./scripts/build.sh --release

# ── Tests ─────────────────────────────────────────────────────────────────────
# Depends on build so that tests are compiled (no-op when nothing changed).
test: build
	ctest --test-dir $(BUILD_DEBUG) --output-on-failure -V

# ── Run ───────────────────────────────────────────────────────────────────────
run: build
ifeq ($(shell uname),Darwin)
	@test -d "$(BUILD_DEBUG)/src/app/Thoth.app" || { echo "Error: Thoth.app bundle not found."; exit 1; }
	open "$(CURDIR)/$(BUILD_DEBUG)/src/app/Thoth.app"
else
	@test -x "$(APP_BIN)" || { echo "Error: $(APP_BIN) not found."; exit 1; }
	"$(CURDIR)/$(APP_BIN)"
endif

# ── Code quality ──────────────────────────────────────────────────────────────
# These targets need cmake to have been configured; use build.ninja as a sentinel
# so that configure is automatic on a clean checkout.
$(BUILD_DEBUG)/build.ninja:
	$(_CMAKE_ENV) cmake --preset debug

lint:
	pre-commit run --all-files

format: $(BUILD_DEBUG)/build.ninja
	$(_CMAKE_ENV) cmake --build --preset debug --target ThothFormat

docs: $(BUILD_DEBUG)/build.ninja
	$(_CMAKE_ENV) cmake --build --preset debug --target ThothDocs

# ── Distribution  (macOS DMG) ─────────────────────────────────────────────────
package:
	VCPKG_ROOT="$(VCPKG_ROOT)" ./scripts/package.sh --version "$(VERSION)"

# ── Cleaning ──────────────────────────────────────────────────────────────────
clean:
	rm -rf $(BUILD_DEBUG)

clean-all:
	rm -rf build/ dist/
