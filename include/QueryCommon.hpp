#ifndef QUERY_COMMON_HPP
#define QUERY_COMMON_HPP

#include "SensorData.hpp"       // For sensor readings
#include "AnomalyDetector.hpp"  // For AnomalyDetector and its thresholds
#include <string> 
#include <sstream> 
#include <iomanip> // For std::fixed, std::setprecision
#include <cmath>   // For std::max
#include <vector>  // Required by SensorData.hpp if it uses std::vector implicitly

// Enum for sorting criteria
enum class SortCriteria {
    TIMESTAMP_ASC,      // Sort by timestamp (ascending)
    TIMESTAMP_DESC,     // Sort by timestamp (descending)
    TEMP_ASC,           // Sort by temperature (ascending)
    TEMP_DESC,          // Sort by temperature (descending)
    HUMIDITY_ASC,       // Sort by humidity (ascending)
    HUMIDITY_DESC,      // Sort by humidity (descending)
    LIGHT_ASC,          // Sort by light intensity (ascending)
    LIGHT_DESC,         // Sort by light intensity (descending)
    DEVIATION_ASC,      // Sort by deviation magnitude (ascending)
    DEVIATION_DESC      // Sort by deviation magnitude (descending)
};

// Structure to hold a sensor reading along with derived information for querying/display.
// Inherits from SensorData to reuse its fields and methods like toString().
struct QueryResult : public SensorData {
    bool isAnomalousFlag;
    double deviationValue; // Value representing how far off the data is from normal thresholds.

    QueryResult(const SensorData& sd, bool isAnomalous, double deviation)
        : SensorData(sd), isAnomalousFlag(isAnomalous), deviationValue(deviation) {} 

    // Extended toString method to include anomaly status and deviation.
    std::string queryResultToString() const {
        std::ostringstream oss;
        oss << SensorData::toString() // Call base class method
            << ", Anomalous: " << (isAnomalousFlag ? "YES" : "NO")
            << ", Deviation: " << std::fixed << std::setprecision(2) << deviationValue;
        return oss.str();
    }
};

// Helper function to calculate deviation.
// Calculates how far a value is outside its "normal" range.
inline double calculate_deviation_metric(const SensorData& data, const AnomalyDetector::AnomalyThresholds& thresholds) {
    double totalDeviation = 0.0;

    // Temperature deviation 
    if (data.temperature < thresholds.minTemp) {
        totalDeviation += (thresholds.minTemp - data.temperature);
    } else if (data.temperature > thresholds.maxTemp) {
        totalDeviation += (data.temperature - thresholds.maxTemp);
    }

    // Humidity deviation
    if (data.humidity < thresholds.minHumidity) {
        totalDeviation += (thresholds.minHumidity - data.humidity);
    } else if (data.humidity > thresholds.maxHumidity) {
        totalDeviation += (data.humidity - thresholds.maxHumidity);
    }

    // Light intensity deviation
    if (data.lightIntensity < thresholds.minLight) {
        totalDeviation += (thresholds.minLight - data.lightIntensity);
    } else if (data.lightIntensity > thresholds.maxLight) {
        totalDeviation += (data.lightIntensity - thresholds.maxLight);
    }
    
    return totalDeviation;
}

#endif // QUERY_COMMON_HPP