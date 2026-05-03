cmake_minimum_required(VERSION 3.20)

# vcpkg toolchain: use Thoth wrapper (sets x64-osx triplet on macOS before vcpkg runs)
if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE
        "${CMAKE_SOURCE_DIR}/cmake/ThothVcpkgToolchain.cmake"
        CACHE STRING "Vcpkg toolchain file" FORCE
    )
endif()

project(Thoth LANGUAGES CXX)

# App version — override via -DTHOTH_VERSION=x.y.z (set by scripts/package.sh)
if(NOT DEFINED THOTH_VERSION)
    file(STRINGS "${CMAKE_SOURCE_DIR}/VERSION" THOTH_VERSION LIMIT_COUNT 1)
    if(NOT THOTH_VERSION)
        set(THOTH_VERSION "0.0.0")
    endif()
endif()

# Packaging flag: when ON, ThothApp builds as a macOS .app bundle
option(THOTH_MACOS_BUNDLE "Build ThothApp as a macOS .app bundle" OFF)

# Global Settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOUIC ON) # automatically generate UI header files from UI files
set(CMAKE_AUTORCC ON) # automatically scanning .qrc files
set(CMAKE_AUTOMOC ON) # automatically scanning header files containing Qt meta-objects
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")    # avoid Unicode character encoding
# issues in Windows
if(MSVC)
    add_link_options(/ignore:4099)
endif()

function(thoth_set_warning target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -pedantic)
    endif()
endfunction()

# Dependencies
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/third_party/whisper.cpp/CMakeLists.txt")
    message(
        FATAL_ERROR
            "whisper.cpp submodule not found. Run 'git submodule update --init --recursive' to initialize it."
    )
endif()

find_package(
    Qt6
    COMPONENTS Core
               Concurrent
               Widgets
               Network
               Multimedia
               Test
    REQUIRED
)
find_package(nlohmann_json REQUIRED)
find_package(google_cloud_cpp_texttospeech REQUIRED)
find_package(spdlog REQUIRED)
find_package(GTest CONFIG REQUIRED)

# exclude examples, tests, and server from Whisper build, for clean build
set(WHISPER_BUILD_EXAMPLES
    OFF
    CACHE BOOL "Build Whisper examples" FORCE
)
set(WHISPER_BUILD_TESTS
    OFF
    CACHE BOOL "Build Whisper tests" FORCE
)
set(WHISPER_BUILD_SERVER
    OFF
    CACHE BOOL "Build Whisper server" FORCE
)

enable_testing()

add_subdirectory(third_party/whisper.cpp EXCLUDE_FROM_ALL SYSTEM)
add_subdirectory(src/core)
add_subdirectory(src/app)
add_subdirectory(test)

# Target-4, Thoth Documentation
add_custom_target(
    ThothDocs
    COMMAND doxygen
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/docs
)

# Target-5, Thoth Clean
add_custom_target(
    ThothClean
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Cleaning build directory..."
)

# Code Formatting
find_program(CLANG_FORMAT clang-format)
if(CLANG_FORMAT)
    file(
        GLOB_RECURSE
        ALL_SOURCE_FILES
        ${CMAKE_SOURCE_DIR}/src/*.cpp
        ${CMAKE_SOURCE_DIR}/src/*.h
        ${CMAKE_SOURCE_DIR}/include/*.h
    )
    add_custom_target(
        ThothFormat
        COMMAND ${CLANG_FORMAT} -i -style=file ${ALL_SOURCE_FILES}
        COMMENT "Formatting code with clang-format..."
    )
else()
    message(STATUS "clang-format not found, code formatting will not be available.")
endif()
