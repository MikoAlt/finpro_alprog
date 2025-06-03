cmake_minimum_required(VERSION 3.10)

project(SensorSystemTests)

# Include GTest
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Add executable for tests
add_executable(run_tests
    test_anomaly_detector.cpp
    test_data_storage.cpp
    test_server.cpp
    test_client.cpp
)

# Link GTest libraries
# Use GTest::gtest_main for automatic main()
target_link_libraries(run_tests
    PRIVATE
    GTest::gtest_main
    finpro_core
    finpro_data_processing
    finpro_storage
    nlohmann_json::nlohmann_json
)

if (WIN32)
    target_link_libraries(run_tests PRIVATE Ws2_32)
endif()

# Include directories for your project headers and GTest headers
# (GTest include is handled by target_link_libraries)
target_include_directories(run_tests
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# Enable test discovery
include(GoogleTest)
gtest_discover_tests(run_tests)
