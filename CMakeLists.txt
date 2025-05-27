cmake_minimum_required(VERSION 3.20) # Increased version for FetchContent best practices
project(finpro LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add include directory
include_directories(include)

# --- Libraries for modules ---
# Data Processing Module
add_library(finpro_data_processing src/data_processing/AnomalyDetector.cpp)
target_include_directories(finpro_data_processing PUBLIC include)

# Storage Module
add_library(finpro_storage src/storage/DataStorage.cpp)
target_include_directories(finpro_storage PUBLIC include)
# Link storage module against nlohmann_json if it uses it directly (it does for tests, but not in lib itself)
# If DataStorage.cpp itself needed nlohmann::json, you would link it here.

# Query & Synchronization Module
add_library(finpro_query_sync src/query_sync/DataManager.cpp)
target_include_directories(finpro_query_sync PUBLIC include)
# Since DataManager.cpp includes AnomalyDetector.hpp and SensorData.hpp from the global 'include' dir,
# and AnomalyDetector itself is a class, finpro_query_sync might conceptually depend on
# finpro_data_processing if it were to use an object of it.
# Here, it creates its own AnomalyDetector instance, so it primarily needs the headers.
# Make sure finpro_data_processing's headers are accessible. The global include_directories(include) should cover this.

# --- Main Executable ---
add_executable(finpro src/main.cpp)
# Link main executable against the module libraries
target_link_libraries(finpro PRIVATE finpro_data_processing finpro_storage, finpro_query_sync)


# --- Google Test Setup ---
# Enable testing
enable_testing()

# Add the tests subdirectory
add_subdirectory(tests)

# --- nlohmann-json (for test verification, and potentially later for DataStorage) ---
# Option 1: FetchContent (Recommended for modern CMake)
include(FetchContent)
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
FetchContent_MakeAvailable(json)

# Make nlohmann_json available to your targets if they need it
# For example, if DataStorage.cpp directly used nlohmann::json:
# target_link_libraries(finpro_storage PUBLIC nlohmann_json::nlohmann_json)
# And for tests (already handled in tests/CMakeLists.txt if linked there)

# Option 2: find_package (If nlohmann-json is installed system-wide or via a package manager)
# find_package(nlohmann_json 3 REQUIRED)
# if(nlohmann_json_FOUND)
#   message(STATUS "Found nlohmann_json: ${nlohmann_json_INCLUDE_DIRS}")
#   include_directories(${nlohmann_json_INCLUDE_DIRS}) # Older way
# else()
#   message(FATAL_ERROR "nlohmann_json not found. Please install it or use FetchContent.")
# endif()

# Note: The tests/CMakeLists.txt will link nlohmann_json to the test runner.
# If your main DataStorage library were to use nlohmann_json directly for its implementation
# (not just for tests), you would link it to the finpro_storage library here:
# target_link_libraries(finpro_storage PUBLIC nlohmann_json::nlohmann_json)
# For now, it's only used in tests for verification.

