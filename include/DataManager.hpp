#ifndef DATA_MANAGER_HPP
#define DATA_MANAGER_HPP

#include "SensorData.hpp"        
#include "AnomalyDetector.hpp"   
#include "QueryCommon.hpp"        // For SortCriteria and QueryResult
#include <vector> 
#include <mutex> 
#include <string>  
#include <optional>               // For optional query parameters
#include <algorithm>              // For std::sort

class DataManager {
public:
    // Constructor takes the anomaly thresholds to internally use an AnomalyDetector
    // and for calculating deviation.
    DataManager(const AnomalyDetector::AnomalyThresholds& thresholds);

    // Adds new sensor data to the historical log. Thread-safe.
    void addSensorData(const SensorData& data);

    // Parameters for querying data
    struct QueryParams {
        std::optional<bool> filterAnomalousOnly; // true = only anomalous, false = only normal, nullopt = all
        SortCriteria sortBy = SortCriteria::TIMESTAMP_ASC; // Default sort order
        // Future extensions:
        // std::optional<std::string> sensorIdFilter;
        // std::optional<std::pair<int64_t, int64_t>> timeRangeFilterMs;
    };

    // Queries the stored sensor data based on the given parameters. Thread-safe.
    std::vector<QueryResult> queryData(const QueryParams& params);

private:
    std::vector<SensorData> historicalData_;
    AnomalyDetector anomalyDetector_; // Instance of AnomalyDetector for checking anomalies
    AnomalyDetector::AnomalyThresholds thresholds_; // Store thresholds for deviation calculation

    mutable std::mutex dataMutex_; // Mutex to protect access to historicalData_

    // Helper to convert SensorData to QueryResult (calculates anomaly status and deviation)
    QueryResult convertToQueryResult(const SensorData& sd) const;
};

#endif // DATA_MANAGER_HPP