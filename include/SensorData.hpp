#ifndef SENSORDATA_HPP
#define SENSORDATA_HPP

#include <string>
#include <chrono>
#include <cstdint> // For int64_t
#include <sstream> // For toString
#include <iomanip> // For std::fixed and std::setprecision

struct SensorData {
    int64_t timestamp_ms; // Milliseconds since epoch
    double temperature;
    double humidity;
    double lightIntensity;

    // Helper to convert from time_point
    static int64_t time_point_to_ms(const std::chrono::system_clock::time_point& tp) {
        return std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()).count();
    }

    // Helper to convert back to time_point
    static std::chrono::system_clock::time_point ms_to_time_point(int64_t ms) {
        return std::chrono::system_clock::time_point(std::chrono::milliseconds(ms));
    }

    static SensorData fromString(const std::string& dataStr);

    std::string toString() const {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2)
            << "Timestamp (ms): " << timestamp_ms
            << ", Temp: " << temperature << " C"
            << ", Humidity: " << humidity << " %"
            << ", Light: " << lightIntensity << " lux";
        return oss.str();
    }

    // For easy comparison in tests
    bool operator==(const SensorData& other) const {
        return timestamp_ms == other.timestamp_ms &&
               temperature == other.temperature && // Consider using epsilon comparison for doubles if needed
               humidity == other.humidity &&
               lightIntensity == other.lightIntensity;
    }
};

#endif //SENSORDATA_HPP
