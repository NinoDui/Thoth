cmake_minimum_required(VERSION 3.20)
project(Thoth LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOUIC ON)   # automatically generate UI header files from UI files
set(CMAKE_AUTORCC ON)   # automatically scanning .qrc files
set(CMAKE_AUTOMOC ON)   # automatically scanning header files containing Qt meta-objects

add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

find_package(Qt6 COMPONENTS Widgets Network Multimedia REQUIRED)

target_include_directories(Thoth PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(Thoth PRIVATE Qt6::Widgets Qt6::Network Qt6::Multimedia)

if(WIN32)
    set_target_properties(Thoth PROPERTIES WIN32_EXECUTABLE ON)
endif()