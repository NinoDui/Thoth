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
    # add_compile_options("/fsanitize=address")
endif()

# Dependencies
find_package(Qt6 COMPONENTS Core Concurrent Widgets Network Multimedia REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(google_cloud_cpp_texttospeech REQUIRED)
find_package(spdlog REQUIRED)
find_package(GTest CONFIG REQUIRED)

add_subdirectory(src/core)
add_subdirectory(src/app)
add_subdirectory(test)

# Target-4, Thoth Documentation
add_custom_target(ThothDocs
    COMMAND doxygen
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/docs
)

# Target-5, Thoth Clean
add_custom_target(ThothClean
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Cleaning build directory..."
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
