#ifndef ANOMALYDETECTOR_HPP
#define ANOMALYDETECTOR_HPP

#include "SensorData.hpp"
#include <vector>

class AnomalyDetector {
public:
    struct AnomalyThresholds {
        double minTemp = 15.0;    // degrees Celsius
        double maxTemp = 30.0;    // degrees Celsius
        double minHumidity = 30.0;  // percentage
        double maxHumidity = 70.0;  // percentage
        double minLight = 100.0;   // lux
        double maxLight = 1000.0;  // lux
    };

    AnomalyDetector(); // Added default constructor
    AnomalyDetector(AnomalyThresholds thresholds); // Removed default argument
    bool isAnomalous(const SensorData& data) const;
    std::vector<SensorData> findAnomalies(const std::vector<SensorData>& dataBatch) const;

private:
    AnomalyThresholds thresholds_;
};

#endif //ANOMALYDETECTOR_HPP
