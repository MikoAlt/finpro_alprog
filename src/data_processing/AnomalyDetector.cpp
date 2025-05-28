#include "AnomalyDetector.hpp"

// Constructor with custom thresholds
AnomalyDetector::AnomalyDetector(AnomalyThresholds thresholds) : thresholds_(thresholds) {}

// Default constructor using default-initialized AnomalyThresholds
AnomalyDetector::AnomalyDetector() : thresholds_(AnomalyThresholds()) {} // Or simply thresholds_() or thresholds_{}

bool AnomalyDetector::isAnomalous(const SensorData& data) const {
    if (data.temperature < thresholds_.minTemp || data.temperature > thresholds_.maxTemp) {
        return true;
    }
    if (data.humidity < thresholds_.minHumidity || data.humidity > thresholds_.maxHumidity) {
        return true;
    }
    if (data.lightIntensity < thresholds_.minLight || data.lightIntensity > thresholds_.maxLight) {
        return true;
    }
    return false;
}

std::vector<SensorData> AnomalyDetector::findAnomalies(const std::vector<SensorData>& dataBatch) const {
    std::vector<SensorData> anomalies;
    for (const auto& data : dataBatch) {
        if (isAnomalous(data)) {
            anomalies.push_back(data);
        }
    }
    return anomalies;
}
