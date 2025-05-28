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
    # Add other test files here
)

# Link GTest libraries
target_link_libraries(run_tests
    PRIVATE
    GTest::gtest
    GTest::gtest_main
)

# Include directories for your project headers and GTest headers
target_include_directories(run_tests
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include # Path to your project's include directory
    ${googletest_SOURCE_DIR}/googletest/include # GTest headers
)

# Link against the source files of the modules being tested
# This assumes your main CMakeLists.txt defines libraries for your modules
# If not, you might need to add the source files directly or adjust paths.
# Example: Link against a library defined in the parent CMakeLists.txt
# target_link_libraries(run_tests PRIVATE YourProjectLibrary)

# If your modules are not built as libraries, you might need to add their source files
# directly to the test executable, or create object libraries for them.
# For simplicity, if AnomalyDetector.cpp and DataStorage.cpp are small and self-contained
# for testing purposes, you could compile them directly into the test runner.
# However, it's better practice to link against compiled libraries from your main project.

# Assuming your main CMakeLists.txt will create libraries for these modules:
target_link_libraries(run_tests PRIVATE
    finpro_data_processing
    finpro_storage
    nlohmann_json::nlohmann_json # Added nlohmann_json target
)


# Enable testing with CTest
include(GoogleTest)
gtest_discover_tests(run_tests)
