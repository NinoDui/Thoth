# Target-3, Thoth Unit Tests
add_executable(ThothTests
    TextParserTest.cpp
    TestRecordController.cpp
    TestLockFreeRingBuffer.cpp
    test_main.cpp
)
if(MSVC OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_link_options(ThothTests PRIVATE /OPT:NOREF)
endif()

# for access to src/core
target_include_directories(ThothTests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/core
)

target_link_libraries(ThothTests PRIVATE
    ThothCore
    GTest::gtest
    GTest::gmock
    Qt6::Core
    Qt6::Test
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
add_custom_target(RunThothTest
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS ThothTests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running unit tests..."
)