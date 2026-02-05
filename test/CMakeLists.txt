# Target-3, Thoth Unit Tests
enable_testing()

# file(GLOB TEST_SRCS test/*.cpp)
SET(TEST_SRCS
    ${CMAKE_SOURCE_DIR}/test/TextParserTest.cpp
)
message(STATUS "TEST_SRCS: ${TEST_SRCS}")
add_executable(ThothTests
    ${TEST_SRCS}
)

target_link_libraries(ThothTests PRIVATE
    ThothCore
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
)

target_compile_definitions(ThothTests PRIVATE
    TEST_RESOURCE_DIR="${CMAKE_SOURCE_DIR}/test/resources"
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