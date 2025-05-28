#include "gtest/gtest.h"
#include "AnomalyDetector.hpp"
#include "SensorData.hpp"
#include <vector>
#include <chrono>

// Test fixture for AnomalyDetector tests
class AnomalyDetectorTest : public ::testing::Test {
protected:
    AnomalyDetector detector_;
    AnomalyDetector::AnomalyThresholds defaultThresholds_;

    SensorData createData(double temp, double hum, double light, int64_t ts_offset_ms = 0) {
        return {
            SensorData::time_point_to_ms(std::chrono::system_clock::now()) + ts_offset_ms,
            temp,
            hum,
            light
        };
    }
};

TEST_F(AnomalyDetectorTest, NoAnomaly) {
    SensorData normalData = createData(25.0, 50.0, 500.0);
    EXPECT_FALSE(detector_.isAnomalous(normalData));
}

TEST_F(AnomalyDetectorTest, TemperatureTooLow) {
    SensorData lowTempData = createData(defaultThresholds_.minTemp - 1.0, 50.0, 500.0);
    EXPECT_TRUE(detector_.isAnomalous(lowTempData));
}

TEST_F(AnomalyDetectorTest, TemperatureTooHigh) {
    SensorData highTempData = createData(defaultThresholds_.maxTemp + 1.0, 50.0, 500.0);
    EXPECT_TRUE(detector_.isAnomalous(highTempData));
}

TEST_F(AnomalyDetectorTest, HumidityTooLow) {
    SensorData lowHumidityData = createData(25.0, defaultThresholds_.minHumidity - 1.0, 500.0);
    EXPECT_TRUE(detector_.isAnomalous(lowHumidityData));
}

TEST_F(AnomalyDetectorTest, HumidityTooHigh) {
    SensorData highHumidityData = createData(25.0, defaultThresholds_.maxHumidity + 1.0, 500.0);
    EXPECT_TRUE(detector_.isAnomalous(highHumidityData));
}

TEST_F(AnomalyDetectorTest, LightIntensityTooLow) {
    SensorData lowLightData = createData(25.0, 50.0, defaultThresholds_.minLight - 1.0);
    EXPECT_TRUE(detector_.isAnomalous(lowLightData));
}

TEST_F(AnomalyDetectorTest, LightIntensityTooHigh) {
    SensorData highLightData = createData(25.0, 50.0, defaultThresholds_.maxLight + 1.0);
    EXPECT_TRUE(detector_.isAnomalous(highLightData));
}

TEST_F(AnomalyDetectorTest, FindAnomaliesInBatch) {
    std::vector<SensorData> batch = {
        createData(25.0, 50.0, 500.0, 0),    // Normal
        createData(10.0, 50.0, 500.0, 1000), // Temp low
        createData(25.0, 80.0, 500.0, 2000), // Humidity high
        createData(28.0, 60.0, 1200.0, 3000) // Light high
    };
    std::vector<SensorData> expectedAnomalies = {
        batch[1], batch[2], batch[3]
    };
    std::vector<SensorData> detectedAnomalies = detector_.findAnomalies(batch);
    ASSERT_EQ(detectedAnomalies.size(), 3);
    EXPECT_EQ(detectedAnomalies[0].temperature, expectedAnomalies[0].temperature);
    EXPECT_EQ(detectedAnomalies[1].humidity, expectedAnomalies[1].humidity);
    EXPECT_EQ(detectedAnomalies[2].lightIntensity, expectedAnomalies[2].lightIntensity);
}

TEST_F(AnomalyDetectorTest, FindAnomaliesEmptyBatch) {
    std::vector<SensorData> batch = {};
    std::vector<SensorData> detectedAnomalies = detector_.findAnomalies(batch);
    EXPECT_TRUE(detectedAnomalies.empty());
}

TEST_F(AnomalyDetectorTest, FindAnomaliesNoAnomaliesInBatch) {
    std::vector<SensorData> batch = {
        createData(25.0, 50.0, 500.0),
        createData(22.0, 55.0, 600.0)
    };
    std::vector<SensorData> detectedAnomalies = detector_.findAnomalies(batch);
    EXPECT_TRUE(detectedAnomalies.empty());
}

TEST_F(AnomalyDetectorTest, CustomThresholds) {
    AnomalyDetector::AnomalyThresholds customThresholds;
    customThresholds.minTemp = 20.0;
    customThresholds.maxTemp = 25.0;
    AnomalyDetector customDetector(customThresholds);

    SensorData normalForDefault = createData(26.0, 50.0, 500.0); // Anomalous for custom
    EXPECT_FALSE(detector_.isAnomalous(normalForDefault)); // Check against default detector
    EXPECT_TRUE(customDetector.isAnomalous(normalForDefault));

    SensorData normalForCustom = createData(22.0, 50.0, 500.0);
    EXPECT_FALSE(customDetector.isAnomalous(normalForCustom));
}
