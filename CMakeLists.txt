cmake_minimum_required(VERSION 3.20)
project(Thoth LANGUAGES CXX)

# Global Settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOUIC ON)   # automatically generate UI header files from UI files
set(CMAKE_AUTORCC ON)   # automatically scanning .qrc files
set(CMAKE_AUTOMOC ON)   # automatically scanning header files containing Qt meta-objects
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")    # avoid Unicode character encoding issues in Windows
if(MSVC)
    add_compile_options("/utf-8")
endif()

# Dependencies
find_package(Qt6 COMPONENTS Core Concurrent Widgets Network Multimedia REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(google_cloud_cpp_texttospeech REQUIRED)
find_package(spdlog REQUIRED)
find_package(GTest CONFIG REQUIRED)

# Target-1, ThothCore Logic (as a static library)

file(GLOB CORE_SRCS src/core/*.cpp)
message(STATUS "CORE_SRCS: ${CORE_SRCS}")

file(GLOB CORE_PUB_HDRS include/thoth/*.h)
message(STATUS "CORE_PUB_HDRS: ${CORE_PUB_HDRS}")

add_library(ThothCore STATIC
    ${CORE_SRCS}
    ${CORE_PUB_HDRS}
)

target_include_directories(ThothCore
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core
)

target_link_libraries(ThothCore
    PUBLIC
        # for dependencies wihch is exposed by headers under include/thoth
        Qt6::Core
        Qt6::Network
        Qt6::Concurrent
        Qt6::Multimedia
        google-cloud-cpp::texttospeech
        spdlog::spdlog
    PRIVATE 
        nlohmann_json::nlohmann_json
)

# Target-2, ThothApp
add_executable(ThothApp
    src/main.cpp
    src/app/Q_AppMainWindow.cpp
)

target_link_libraries(ThothApp PRIVATE
    ThothCore
    Qt6::Widgets
)

if(WIN32)
    # set_target_properties(ThothApp PROPERTIES WIN32_EXECUTABLE ON)
endif()

# Target-3, Thoth Unit Tests
enable_testing()

# file(GLOB TEST_SRCS test/*.cpp)
SET(TEST_SRCS
    test/TextParserTest.cpp
)
message(STATUS "TEST_SRCS: ${TEST_SRCS}")
add_executable(ThothTests
    ${TEST_SRCS}
)

target_include_directories(ThothTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(ThothTests PRIVATE
    ThothCore
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
)

target_compile_definitions(ThothTests PRIVATE
    TEST_RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test/resources"
)

include(GoogleTest)
include(CTest)

# Discover and register tests with CTest
gtest_discover_tests(ThothTests
    DISCOVER_TESTS_DISCOVERY_MODE PRE_TEST
    PROPERTIES
        TIMEOUT 30
)

# Add custom test target for convenience
add_custom_target(ThothTest
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS ThothTests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running unit tests..."
)

# Target-4, Thoth Documentation
add_custom_target(ThothDocs
    COMMAND doxygen
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/docs
)

# Target-5, Thoth Clean
add_custom_target(ThothClean
    COMMAND rm -rf ${CMAKE_CURRENT_BINARY_DIR}
)

# Code Formatting
find_program(CLANG_FORMAT clang-format)
if (CLANG_FORMAT)
    file(GLOB_RECURSE ALL_SOURCE_FILES
        ${CMAKE_SOURCE_DIR}/src/*.cpp
        ${CMAKE_SOURCE_DIR}/src/*.h
        ${CMAKE_SOURCE_DIR}/include/*.h
    )
    add_custom_target(ThothFormat
        COMMAND ${CLANG_FORMAT} -i -style=file ${ALL_SOURCE_FILES}
        COMMENT "Formatting code with clang-format..."
    )
else()
    message(STATUS "clang-format not found, code formatting will not be available.")
endif()
