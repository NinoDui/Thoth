# Thoth vcpkg toolchain wrapper
# Sets VCPKG_TARGET_TRIPLET and VCPKG_HOST_TRIPLET before vcpkg.cmake runs.
# Workaround: grpc fails on arm64-osx (_gRPC_CPP_PLUGIN-NOTFOUND). Use x64-osx on Apple Silicon (runs via Rosetta 2).
# Both target and host triplets must be x64-osx to avoid arm64 host tool builds that also fail.
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    set(VCPKG_TARGET_TRIPLET "x64-osx" CACHE STRING "Force x64-osx on macOS (grpc arm64 workaround)" FORCE)
    set(VCPKG_HOST_TRIPLET "x64-osx" CACHE STRING "Force x64-osx host tools on macOS (grpc arm64 workaround)" FORCE)
        if(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64")
        set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "Build for x86_64 to match vcpkg x64-osx triplet" FORCE)
    endif()
endif()

 # Merge $ENV{CMAKE_PREFIX_PATH} into CMake's variable before vcpkg.cmake overwrites it
if(DEFINED ENV{CMAKE_PREFIX_PATH})
    string(REPLACE ":" ";" _env_prefix "$ENV{CMAKE_PREFIX_PATH}")
    list(APPEND CMAKE_PREFIX_PATH ${_env_prefix})
endif()

include("$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
