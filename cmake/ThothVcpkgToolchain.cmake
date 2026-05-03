# Thoth vcpkg toolchain wrapper Sets VCPKG_TARGET_TRIPLET and VCPKG_HOST_TRIPLET before vcpkg.cmake
# runs. Workaround: grpc fails on arm64-osx (_gRPC_CPP_PLUGIN-NOTFOUND). Use x64-osx on Apple
# Silicon (runs via Rosetta 2). Both target and host triplets must be x64-osx to avoid arm64 host
# tool builds that also fail.
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    set(VCPKG_TARGET_TRIPLET
        "arm64-osx"
        CACHE STRING "Native arm64 on macOS" FORCE
    )
    set(VCPKG_HOST_TRIPLET
        "arm64-osx"
        CACHE STRING "Native arm64 host tools on macOS" FORCE
    )
endif()

# Merge $ENV{CMAKE_PREFIX_PATH} into CMake's variable before vcpkg.cmake overwrites it
if(DEFINED ENV{CMAKE_PREFIX_PATH}) # cmake-lint: disable=W0106
    string(REPLACE ":" ";" _env_prefix "$ENV{CMAKE_PREFIX_PATH}")
    list(APPEND CMAKE_PREFIX_PATH ${_env_prefix})
endif()

include("$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
