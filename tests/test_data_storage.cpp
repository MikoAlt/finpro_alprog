#include "gtest/gtest.h"
#include "DataStorage.hpp"
#include "SensorData.hpp"
#include <vector>
#include <fstream>
#include <cstdio> // For std::remove
#include <chrono>
#include <nlohmann/json.hpp> // For parsing JSON for verification

// Helper to create SensorData for tests
SensorData createTestData(int64_t ts_offset_ms, double temp, double hum, double light) {
    return {
        SensorData::time_point_to_ms(std::chrono::system_clock::now()) + ts_offset_ms,
        temp,
        hum,
        light
    };
}

class DataStorageTest : public ::testing::Test {
protected:
    const std::string testBinaryFile_ = "test_sensor_data.bin";
    const std::string testJsonReportFile_ = "test_anomalies.json";
    DataStorage storage_;

    DataStorageTest() : storage_(testBinaryFile_, testJsonReportFile_) {}

    void SetUp() override {
        // Ensure files are clean before each test
        std::remove(testBinaryFile_.c_str());
        std::remove(testJsonReportFile_.c_str());
    }

    void TearDown() override {
        // Clean up files after each test
        std::remove(testBinaryFile_.c_str());
        std::remove(testJsonReportFile_.c_str());
    }

    // Helper to check if file exists
    bool fileExists(const std::string& filename) {
        std::ifstream f(filename.c_str());
        return f.good();
    }
};

TEST_F(DataStorageTest, StoreSingleDataPoint) {
    SensorData data1 = createTestData(0, 22.5, 45.5, 300.0);
    EXPECT_TRUE(storage_.storeData(data1));
    EXPECT_TRUE(fileExists(testBinaryFile_));

    std::vector<SensorData> loadedData = storage_.loadAllData();
    ASSERT_EQ(loadedData.size(), 1);
    EXPECT_EQ(loadedData[0], data1);
}

TEST_F(DataStorageTest, StoreDataBatch) {
    std::vector<SensorData> batch = {
        createTestData(0, 22.5, 45.5, 300.0),
        createTestData(1000, 23.0, 46.0, 310.0),
        createTestData(2000, 21.5, 45.0, 290.0)
    };
    EXPECT_TRUE(storage_.storeDataBatch(batch));
    EXPECT_TRUE(fileExists(testBinaryFile_));

    std::vector<SensorData> loadedData = storage_.loadAllData();
    ASSERT_EQ(loadedData.size(), batch.size());
    for (size_t i = 0; i < batch.size(); ++i) {
        EXPECT_EQ(loadedData[i], batch[i]);
    }
}

TEST_F(DataStorageTest, LoadAllDataEmptyFile) {
    std::vector<SensorData> loadedData = storage_.loadAllData();
    EXPECT_TRUE(loadedData.empty());
}

TEST_F(DataStorageTest, LoadAllDataNonExistentFile) {
    // File doesn't exist yet, should return empty vector without error
    std::vector<SensorData> loadedData = storage_.loadAllData();
    EXPECT_TRUE(loadedData.empty());
}

TEST_F(DataStorageTest, StoreAndLoadMultipleBatches) {
    SensorData data1 = createTestData(0, 22.5, 45.5, 300.0);
    EXPECT_TRUE(storage_.storeData(data1));

    std::vector<SensorData> batch1 = {
        createTestData(1000, 23.0, 46.0, 310.0),
        createTestData(2000, 21.5, 45.0, 290.0)
    };
    EXPECT_TRUE(storage_.storeDataBatch(batch1));

    SensorData data2 = createTestData(3000, 24.0, 47.0, 320.0);
    EXPECT_TRUE(storage_.storeData(data2));

    std::vector<SensorData> loadedData = storage_.loadAllData();
    ASSERT_EQ(loadedData.size(), 4);
    EXPECT_EQ(loadedData[0], data1);
    EXPECT_EQ(loadedData[1], batch1[0]);
    EXPECT_EQ(loadedData[2], batch1[1]);
    EXPECT_EQ(loadedData[3], data2);
}

TEST_F(DataStorageTest, ExportAnomaliesToJsonEmpty) {
    std::vector<SensorData> anomalies = {};
    EXPECT_TRUE(storage_.exportAnomaliesToJson(anomalies));
    EXPECT_TRUE(fileExists(testJsonReportFile_));

    std::ifstream jsonFile(testJsonReportFile_);
    nlohmann::json j;
    jsonFile >> j;
    EXPECT_TRUE(j.is_array());
    EXPECT_TRUE(j.empty());
}

TEST_F(DataStorageTest, ExportAnomaliesToJsonSingle) {
    std::vector<SensorData> anomalies = {
        createTestData(0, 35.0, 80.0, 50.0) // Anomalous data
    };
    EXPECT_TRUE(storage_.exportAnomaliesToJson(anomalies));
    EXPECT_TRUE(fileExists(testJsonReportFile_));

    std::ifstream jsonFile(testJsonReportFile_);
    nlohmann::json j;
    jsonFile >> j;
    ASSERT_TRUE(j.is_array());
    ASSERT_EQ(j.size(), 1);
    EXPECT_EQ(j[0]["temperature"].get<double>(), anomalies[0].temperature);
    EXPECT_EQ(j[0]["humidity"].get<double>(), anomalies[0].humidity);
    EXPECT_EQ(j[0]["lightIntensity"].get<double>(), anomalies[0].lightIntensity);
    EXPECT_EQ(j[0]["timestamp_ms"].get<int64_t>(), anomalies[0].timestamp_ms);
}

TEST_F(DataStorageTest, ExportAnomaliesToJsonMultiple) {
    std::vector<SensorData> anomalies = {
        createTestData(0, 35.0, 80.0, 50.0),
        createTestData(1000, 10.0, 20.0, 1500.0)
    };
    EXPECT_TRUE(storage_.exportAnomaliesToJson(anomalies));
    EXPECT_TRUE(fileExists(testJsonReportFile_));

    std::ifstream jsonFile(testJsonReportFile_);
    nlohmann::json j;
    jsonFile >> j;
    ASSERT_TRUE(j.is_array());
    ASSERT_EQ(j.size(), 2);
    for(size_t i=0; i<anomalies.size(); ++i) {
        EXPECT_EQ(j[i]["temperature"].get<double>(), anomalies[i].temperature);
        EXPECT_EQ(j[i]["humidity"].get<double>(), anomalies[i].humidity);
        EXPECT_EQ(j[i]["lightIntensity"].get<double>(), anomalies[i].lightIntensity);
        EXPECT_EQ(j[i]["timestamp_ms"].get<int64_t>(), anomalies[i].timestamp_ms);
    }
}

// Test for handling non-existent binary file during load (should be graceful)
TEST_F(DataStorageTest, LoadFromNonExistentBinaryFile) {
    std::remove(testBinaryFile_.c_str()); // Ensure it's gone
    std::vector<SensorData> data = storage_.loadAllData();
    EXPECT_TRUE(data.empty());
}

// Test for handling non-writable JSON file path (difficult to simulate robustly without changing permissions)
// This test mostly checks if the function returns false, actual file system error handling is OS dependent.
TEST_F(DataStorageTest, ExportToNonWritableJsonPath) {
    // Using a path that's typically not writable by a normal user without admin rights
    // This is a heuristic and might pass on some systems if the test runner has elevated privileges.
    DataStorage protectedStorage("test.bin", "/hopefully_non_writable_path/report.json");
    std::vector<SensorData> anomalies = {createTestData(0, 35.0, 80.0, 50.0)};
    // We expect this to fail to open the file for writing.
    // The exact behavior of ofstream on failure can vary (e.g., exception or just failbit)
    // but our function should return false.
    EXPECT_FALSE(protectedStorage.exportAnomaliesToJson(anomalies));
    std::remove("/hopefully_non_writable_path/report.json"); // Clean up if it somehow got created
}

// Test for handling non-writable binary file path for storing data
TEST_F(DataStorageTest, StoreToNonWritableBinaryPath) {
    DataStorage protectedStorage("/hopefully_non_writable_path/data.bin", "report.json");
    SensorData data = createTestData(0, 22.5, 45.5, 300.0);
    EXPECT_FALSE(protectedStorage.storeData(data));
    std::remove("/hopefully_non_writable_path/data.bin");
}
