# CMakeLists for ThothCore, a static library

file(GLOB_RECURSE CORE_SRCS ${CMAKE_SOURCE_DIR}/src/core/*.cpp)
message(STATUS "CORE_SRCS: ${CORE_SRCS}")

# Configuration Phase Evaluation
set(CORE_PUB_HDR_DIR ${CMAKE_SOURCE_DIR}/include)
message(STATUS "CORE_PUB_HDR_DIR: ${CORE_PUB_HDR_DIR}")

file(GLOB CORE_PUB_HDRS ${CORE_PUB_HDR_DIR}/thoth/*.h)
message(STATUS "CORE_PUB_HDRS: ${CORE_PUB_HDRS}")

file(GLOB CORE_INT_HDRS ${CMAKE_SOURCE_DIR}/src/core/internal/*.h)
message(STATUS "CORE_INT_HDRS: ${CORE_INT_HDRS}")

set(CORE_SRC_FILES
    ${CORE_SRCS}
    ${CORE_PUB_HDRS}
    ${CORE_INT_HDRS}
)

add_library(ThothCore STATIC ${CORE_SRC_FILES})

target_include_directories(ThothCore
PUBLIC
    # Generator Expression, available and evaluated in Generation Phase
    $<BUILD_INTERFACE:${CORE_PUB_HDR_DIR}>
PRIVATE
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/core/internal
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