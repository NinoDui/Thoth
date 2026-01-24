cmake_minimum_required(VERSION 3.20)
project(Thoth LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 COMPONENTS Widgets Network Multimedia REQUIRED)

target_link_libraries(Thoth PRIVATE Qt6::Widgets Qt6::Network Qt6::Multimedia)

if(WIN32)
    set_target_properties(Thoth PROPERTIES WIN32_EXECUTABLE ON)
endif()