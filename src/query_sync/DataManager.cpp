#include "DataManager.hpp" // Corresponding header
#include <iostream>        // For potential debug logging

// Constructor
DataManager::DataManager(const AnomalyDetector::AnomalyThresholds& thresholds)
    : anomalyDetector_(thresholds), thresholds_(thresholds) {
    // The anomalyDetector_ is initialized with the provided thresholds.
    // thresholds_ is also stored separately if calculate_deviation_metric needs it directly.
}

void DataManager::addSensorData(const SensorData& data) {
    std::lock_guard<std::mutex> lock(dataMutex_); // RAII style lock
    historicalData_.push_back(data);
    // For debugging:
    // std::cout << "DataManager: Added data - Timestamp: " << data.timestamp_ms << std::endl;
}

QueryResult DataManager::convertToQueryResult(const SensorData& sd) const {
    // AnomalyDetector::isAnomalous is const, so it can be called here.
    bool isAnomalous = anomalyDetector_.isAnomalous(sd);
    // calculate_deviation_metric is a free function in QueryCommon.hpp
    double deviation = calculate_deviation_metric(sd, thresholds_);
    return QueryResult(sd, isAnomalous, deviation);
}

std::vector<QueryResult> DataManager::queryData(const QueryParams& params) {
    std::lock_guard<std::mutex> lock(dataMutex_); // Ensure thread-safe read access

    std::vector<QueryResult> processedResults;
    processedResults.reserve(historicalData_.size());

    // Step 1: Convert SensorData to QueryResult and apply filters
    for (const auto& sensor_data_point : historicalData_) {
        QueryResult query_result_item = convertToQueryResult(sensor_data_point);

        // Apply filter: filterAnomalousOnly
        if (params.filterAnomalousOnly.has_value()) {
            if (params.filterAnomalousOnly.value() != query_result_item.isAnomalousFlag) {
                continue; // Skip if it doesn't match the anomalous filter
            }
        }
        
        // Apply other filters (e.g., sensorId, timeRange) here if they were added to QueryParams

        processedResults.push_back(query_result_item);
    }

    // Step 2: Sort the filtered results
    std::sort(processedResults.begin(), processedResults.end(),
        [&](const QueryResult& a, const QueryResult& b) {
            switch (params.sortBy) {
                case SortCriteria::TIMESTAMP_ASC:
                    return a.timestamp_ms < b.timestamp_ms;
                case SortCriteria::TIMESTAMP_DESC:
                    return a.timestamp_ms > b.timestamp_ms;
                case SortCriteria::TEMP_ASC:
                    return a.temperature < b.temperature;
                case SortCriteria::TEMP_DESC:
                    return a.temperature > b.temperature;
                case SortCriteria::HUMIDITY_ASC:
                    return a.humidity < b.humidity;
                case SortCriteria::HUMIDITY_DESC:
                    return a.humidity > b.humidity;
                case SortCriteria::LIGHT_ASC:
                    return a.lightIntensity < b.lightIntensity;
                case SortCriteria::LIGHT_DESC:
                    return a.lightIntensity > b.lightIntensity;
                case SortCriteria::DEVIATION_ASC:
                    return a.deviationValue < b.deviationValue;
                case SortCriteria::DEVIATION_DESC:
                    return a.deviationValue > b.deviationValue;
                default: // Default to timestamp ascending
                    return a.timestamp_ms < b.timestamp_ms;
            }
        });

    return processedResults;
}