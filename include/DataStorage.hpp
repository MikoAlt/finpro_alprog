#ifndef DATASTORAGE_HPP
#define DATASTORAGE_HPP

#include "SensorData.hpp"
#include <vector>
#include <string>
#include <fstream>

class DataStorage {
public:
    DataStorage(const std::string& binaryFilePath, const std::string& jsonReportPath);

    // Appends a single data point to the binary file
    bool storeData(const SensorData& data);
    // Appends a batch of data points to the binary file
    bool storeDataBatch(const std::vector<SensorData>& dataBatch);
    // Replaces all data in the binary file with the provided batch
    bool replaceAllData(const std::vector<SensorData>& dataBatch);
    // Loads all data from the binary file
    std::vector<SensorData> loadAllData();
    // Exports a list of anomalies to a JSON file
    bool exportAnomaliesToJson(const std::vector<SensorData>& anomalies);

private:
    std::string binaryFilePath_;
    std::string jsonReportPath_;

    // Helper for simple JSON generation for a single SensorData item
    std::string sensorDataToJson(const SensorData& data) const;
};

#endif //DATASTORAGE_HPP
