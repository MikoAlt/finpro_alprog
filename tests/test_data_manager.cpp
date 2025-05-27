#include "gtest/gtest.h"      // Google Test header
#include "DataManager.hpp"    // The class we are testing
#include "SensorData.hpp"     // For creating SensorData objects
#include "QueryCommon.hpp"    // For QueryResult, SortCriteria
#include "AnomalyDetector.hpp"// For AnomalyThresholds

#include <vector>
#include <memory>      // For std::unique_ptr
#include <algorithm>   // For std::all_of, std::find_if etc.
#include <chrono>      // For creating timestamps for test data

// Test Fixture for DataManager tests
class DataManagerTest : public ::testing::Test {
protected:
    AnomalyDetector::AnomalyThresholds defaultThresholds;
    std::unique_ptr<DataManager> dm;

    // Constructor for the fixture (can be used for one-time setup for the fixture)
    DataManagerTest() {
        // Standard thresholds used in the system for consistency
        defaultThresholds.minTemp = 15.0;
        defaultThresholds.maxTemp = 30.0;
        defaultThresholds.minHumidity = 30.0;
        defaultThresholds.maxHumidity = 70.0;
        defaultThresholds.minLight = 100.0;
        defaultThresholds.maxLight = 1000.0;
    }

    // SetUp is called before each TEST_F case
    void SetUp() override {
        dm = std::make_unique<DataManager>(defaultThresholds);
    }

    // TearDown is called after each TEST_F case (if needed for explicit cleanup)
    // void TearDown() override {}

    // Helper to create SensorData for tests, ensuring somewhat unique timestamps
    // by using an offset from a fixed base time.
    SensorData createData(int64_t ts_offset_ms, double temp, double hum, double light) {
        // Using a fixed base time for reproducibility of timestamps in tests
        static const auto base_tp = std::chrono::system_clock::from_time_t(1700000000L); // An arbitrary fixed point in time
        int64_t timestamp = SensorData::time_point_to_ms(base_tp + std::chrono::milliseconds(ts_offset_ms));
        return {timestamp, temp, hum, light};
    }
};

// Test case: Querying an empty DataManager should return no results
TEST_F(DataManagerTest, QueryEmptyDataManager) {
    DataManager::QueryParams params;
    std::vector<QueryResult> results = dm->queryData(params);
    EXPECT_TRUE(results.empty());
}

// Test case: Adding a single normal data point and querying it
TEST_F(DataManagerTest, AddAndQuerySingleNormalItem) {
    SensorData normalData = createData(0, 22.0, 55.0, 400.0); // Well within normal ranges
    dm->addSensorData(normalData);

    DataManager::QueryParams params; // Default query (no filters, sort by timestamp)
    std::vector<QueryResult> results = dm->queryData(params);

    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].timestamp_ms, normalData.timestamp_ms);
    EXPECT_DOUBLE_EQ(results[0].temperature, normalData.temperature); // Use for floating point
    EXPECT_FALSE(results[0].isAnomalousFlag);
    // For perfectly normal data matching center of ranges, deviation could be 0.
    // Here, it's normal, so deviation should be relatively small or zero.
    // The exact value depends on the calculate_deviation_metric and thresholds.
}

// Test case: Adding a single anomalous data point (low temperature)
TEST_F(DataManagerTest, AddAndQuerySingleAnomalousItemLowTemp) {
    SensorData anomalousData = createData(10, 10.0, 50.0, 300.0); // Temp below minTemp
    dm->addSensorData(anomalousData);

    DataManager::QueryParams params;
    std::vector<QueryResult> results = dm->queryData(params);

    ASSERT_EQ(results.size(), 1);
    EXPECT_TRUE(results[0].isAnomalousFlag);
    EXPECT_GT(results[0].deviationValue, 0.0); // Deviation should be greater than 0
}

// Test case: Filtering to get only anomalous data
TEST_F(DataManagerTest, FilterAnomalousOnly) {
    dm->addSensorData(createData(0, 25.0, 50.0, 300.0));   // Normal
    dm->addSensorData(createData(10, 10.0, 50.0, 300.0));  // Anomalous (temp low)
    dm->addSensorData(createData(20, 20.0, 80.0, 300.0));  // Anomalous (humidity high)
    dm->addSensorData(createData(30, 22.0, 60.0, 1500.0)); // Anomalous (light high)
    dm->addSensorData(createData(40, 28.0, 65.0, 900.0));  // Normal

    DataManager::QueryParams params;
    params.filterAnomalousOnly = true;
    std::vector<QueryResult> results = dm->queryData(params);

    ASSERT_EQ(results.size(), 3); // Expecting 3 anomalous items
    for (const auto& res : results) {
        EXPECT_TRUE(res.isAnomalousFlag);
    }
}

// Test case: Filtering to get only normal data
TEST_F(DataManagerTest, FilterNormalOnly) {
    dm->addSensorData(createData(0, 25.0, 50.0, 300.0));   // Normal
    dm->addSensorData(createData(10, 10.0, 50.0, 300.0));  // Anomalous (temp low)
    dm->addSensorData(createData(20, 20.0, 40.0, 150.0));  // Normal
    dm->addSensorData(createData(30, 22.0, 60.0, 1500.0)); // Anomalous (light high)

    DataManager::QueryParams params;
    params.filterAnomalousOnly = false; // Get only normal data
    std::vector<QueryResult> results = dm->queryData(params);

    ASSERT_EQ(results.size(), 2); // Expecting 2 normal items
    for (const auto& res : results) {
        EXPECT_FALSE(res.isAnomalousFlag);
    }
}

// Test case: Sorting data by temperature in descending order
TEST_F(DataManagerTest, SortByTemperatureDescending) {
    dm->addSensorData(createData(0, 20.0, 50.0, 300.0));  // Mid temp
    dm->addSensorData(createData(10, 28.0, 50.0, 300.0)); // High temp
    dm->addSensorData(createData(20, 16.0, 50.0, 300.0)); // Low temp

    DataManager::QueryParams params;
    params.sortBy = SortCriteria::TEMP_DESC;
    std::vector<QueryResult> results = dm->queryData(params);

    ASSERT_EQ(results.size(), 3);
    EXPECT_DOUBLE_EQ(results[0].temperature, 28.0);
    EXPECT_DOUBLE_EQ(results[1].temperature, 20.0);
    EXPECT_DOUBLE_EQ(results[2].temperature, 16.0);
}

// Test case: Sorting data by deviation in descending order
TEST_F(DataManagerTest, SortByDeviationDescending) {
    // Normal data, very low deviation
    dm->addSensorData(createData(0, defaultThresholds.minTemp + (defaultThresholds.maxTemp - defaultThresholds.minTemp)/2.0, 50.0, 300.0));
    // Highly anomalous data (temp very low), should have high deviation
    SensorData highDevData = createData(10, 5.0, 50.0, 300.0);
    dm->addSensorData(highDevData);
    // Moderately anomalous data (humidity high)
    dm->addSensorData(createData(20, 20.0, defaultThresholds.maxHumidity + 10.0, 300.0));

    DataManager::QueryParams params;
    params.sortBy = SortCriteria::DEVIATION_DESC;
    std::vector<QueryResult> results = dm->queryData(params);

    ASSERT_EQ(results.size(), 3);
    // Check that deviation values are sorted in descending order
    EXPECT_GE(results[0].deviationValue, results[1].deviationValue);
    EXPECT_GE(results[1].deviationValue, results[2].deviationValue);
    // The one with temp 5.0 should have the highest deviation
    EXPECT_DOUBLE_EQ(results[0].temperature, highDevData.temperature);
}

// Test case: Combination of filter and sort
TEST_F(DataManagerTest, FilterAnomalousAndSortByLightAscending) {
    dm->addSensorData(createData(0, 25.0, 50.0, 300.0));     // Normal
    dm->addSensorData(createData(10, 5.0, 50.0, 1500.0));    // Anomalous (temp), high light
    dm->addSensorData(createData(20, 20.0, 80.0, 50.0));     // Anomalous (humidity), low light
    dm->addSensorData(createData(30, 18.0, 60.0, 1200.0));   // Anomalous (temp), mid light

    DataManager::QueryParams params;
    params.filterAnomalousOnly = true;
    params.sortBy = SortCriteria::LIGHT_ASC;
    std::vector<QueryResult> results = dm->queryData(params);

    ASSERT_EQ(results.size(), 3); // 3 anomalous items
    // Check they are sorted by light intensity ascending
    EXPECT_DOUBLE_EQ(results[0].lightIntensity, 50.0);
    EXPECT_DOUBLE_EQ(results[1].lightIntensity, 1200.0);
    EXPECT_DOUBLE_EQ(results[2].lightIntensity, 1500.0);

    // Also ensure all results are indeed anomalous
    for (const auto& res : results) {
        EXPECT_TRUE(res.isAnomalousFlag);
    }
}