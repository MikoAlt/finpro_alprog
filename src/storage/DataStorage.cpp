#include "DataStorage.hpp"
#include <iostream> // For error logging, consider a more robust logging mechanism for production
#include <iomanip> // For std::fixed and std::setprecision in JSON
#include <sstream> // For JSON string building

DataStorage::DataStorage(const std::string& binaryFilePath, const std::string& jsonReportPath)
    : binaryFilePath_(binaryFilePath), jsonReportPath_(jsonReportPath) {}

bool DataStorage::storeData(const SensorData& data) {
    std::ofstream outFile(binaryFilePath_, std::ios::binary | std::ios::app);
    if (!outFile) {
        // std::cerr << "Error opening binary file for writing: " << binaryFilePath_ << std::endl;
        return false;
    }
    outFile.write(reinterpret_cast<const char*>(&data), sizeof(SensorData));
    outFile.close();
    return !outFile.fail(); // Check if write was successful before closing
}

bool DataStorage::storeDataBatch(const std::vector<SensorData>& dataBatch) {
    std::ofstream outFile(binaryFilePath_, std::ios::binary | std::ios::app);
    if (!outFile) {
        // std::cerr << "Error opening binary file for writing batch: " << binaryFilePath_ << std::endl;
        return false;
    }
    for (const auto& data : dataBatch) {
        outFile.write(reinterpret_cast<const char*>(&data), sizeof(SensorData));
        if (outFile.fail()) { // Check after each write
            // std::cerr << "Error writing data to binary file: " << binaryFilePath_ << std::endl;
            outFile.close();
            return false;
        }
    }
    outFile.close();
    return true;
}

bool DataStorage::replaceAllData(const std::vector<SensorData>& dataBatch) {
    // Open file in truncate mode to replace all content
    std::ofstream outFile(binaryFilePath_, std::ios::binary | std::ios::trunc);
    if (!outFile) {
        // std::cerr << "Error opening binary file for writing batch: " << binaryFilePath_ << std::endl;
        return false;
    }
    for (const auto& data : dataBatch) {
        outFile.write(reinterpret_cast<const char*>(&data), sizeof(SensorData));
        if (outFile.fail()) { // Check after each write
            // std::cerr << "Error writing data to binary file: " << binaryFilePath_ << std::endl;
            outFile.close();
            return false;
        }
    }
    outFile.close();
    return true;
}

std::vector<SensorData> DataStorage::loadAllData() {
    std::vector<SensorData> allData;
    std::ifstream inFile(binaryFilePath_, std::ios::binary);
    if (!inFile) {
        // std::cerr << "Error opening binary file for reading: " << binaryFilePath_ << std::endl;
        return allData; // Return empty vector
    }
    SensorData dataPoint;
    while (inFile.read(reinterpret_cast<char*>(&dataPoint), sizeof(SensorData))) {
        allData.push_back(dataPoint);
    }
    inFile.close();
    return allData;
}

std::string DataStorage::sensorDataToJson(const SensorData& data) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    oss << "  {\n";
    oss << "    \"timestamp_ms\": " << data.timestamp_ms << ",\n";
    oss << "    \"temperature\": " << data.temperature << ",\n";
    oss << "    \"humidity\": " << data.humidity << ",\n";
    oss << "    \"lightIntensity\": " << data.lightIntensity << "\n";
    oss << "  }";
    return oss.str();
}

bool DataStorage::exportAnomaliesToJson(const std::vector<SensorData>& anomalies) {
    std::ofstream jsonFile(jsonReportPath_);
    if (!jsonFile) {
        // std::cerr << "Error opening JSON file for writing: " << jsonReportPath_ << std::endl;
        return false;
    }
    jsonFile << "[\n";
    for (size_t i = 0; i < anomalies.size(); ++i) {
        jsonFile << sensorDataToJson(anomalies[i]);
        if (i < anomalies.size() - 1) {
            jsonFile << ",\n";
        }
    }
    jsonFile << "\n]\n";
    jsonFile.close();
    return !jsonFile.fail();
}
