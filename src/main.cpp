// New enhanced main.cpp with server/client modes
#include "SensorData.hpp"        
#include "AnomalyDetector.hpp"   
#include "DataManager.hpp"       
#include "QueryCommon.hpp"       
#include "Server.hpp"
#include "Client.hpp"
#include "DataStorage.hpp"

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <iomanip>
#include <chrono>
#include <thread>

// Helper function to print query results neatly
void printQueryResults(const std::vector<QueryResult>& results) {
    if (results.empty()) {
        std::cout << "No data matching the specified criteria.\n";
        return;
    }

    std::cout << "\n--- Query Results --- \n";
    std::cout << std::left
              << std::setw(20) << "Timestamp (ms)"
              << std::setw(12) << "Temp (C)"
              << std::setw(12) << "Hum (%)"
              << std::setw(15) << "Light (lx)"
              << std::setw(12) << "Anomalous"
              << std::setw(12) << "Deviation" << std::endl;
    std::cout << std::string(90, '-') << std::endl;

    for (const auto& qr : results) {
        std::cout << std::left
                  << std::setw(20) << qr.timestamp_ms
                  << std::fixed << std::setprecision(2)
                  << std::setw(12) << qr.temperature
                  << std::setw(12) << qr.humidity
                  << std::setw(15) << qr.lightIntensity
                  << std::setw(12) << (qr.isAnomalousFlag ? "YES" : "NO")
                  << std::setw(12) << qr.deviationValue << std::endl;
    }
    std::cout << std::string(90, '-') << std::endl << std::endl;
}

void displayHelp() {
    std::cout << "\nSmart Classroom Monitoring CLI (Part 4 - Query & Sync)\n";
    std::cout << "-------------------------------------------------------\n";
    std::cout << "Available Commands:\n";
    std::cout << "  add <timestamp_ms> <temp> <humidity> <light_intensity>\n";
    std::cout << "    Adds a new sensor reading. Timestamp is milliseconds since epoch.\n";
    std::cout << "    Example: add 1678886400000 25.5 50.2 300.0\n\n";
    std::cout << "  query [anomalous | normal] [sort <criteria>]\n";
    std::cout << "    Queries stored sensor data. All parts are optional.\n";
    std::cout << "    - [anomalous | normal]: Filter by anomaly status.\n";
    std::cout << "    - [sort <criteria>]: Sort results. Criteria include:\n";
    std::cout << "        ts_asc, ts_desc (timestamp)\n";
    std::cout << "        temp_asc, temp_desc (temperature)\n";
    std::cout << "        hum_asc, hum_desc (humidity)\n";
    std::cout << "        light_asc, light_desc (light intensity)\n";
    std::cout << "        dev_asc, dev_desc (deviation magnitude)\n";
    std::cout << "    Example: query anomalous sort dev_desc\n";
    std::cout << "    Example: query sort ts_asc\n\n";
    std::cout << "  help   - Shows this help message.\n";
    std::cout << "  exit   - Exits the CLI application.\n\n";
}

// Server mode function
int runServerMode(int port) {
    std::cout << "Starting Smart Classroom Monitoring Server on port " << port << std::endl;
    
    AnomalyDetector::AnomalyThresholds thresholds;
    DataManager dataManager(thresholds);
    DataStorage dataStorage("sensor_data.bin", "anomaly_report.json");
    
    Server server(port, &dataManager, &dataStorage);
    
    // Set up real-time anomaly notification
    server.setDataCallback([&](const SensorData& data) {
        AnomalyDetector detector(thresholds);
        if (detector.isAnomalous(data)) {
            std::cout << "ANOMALY DETECTED: " << data.toString() << std::endl;
        } else {
            std::cout << "Normal reading: " << data.toString() << std::endl;
        }
    });
    
    server.start();
    
    std::cout << "Server is running. Press Enter to stop..." << std::endl;
    std::cin.get();
    
    server.stop();
    
    // Export anomalies to JSON
    DataManager::QueryParams params;
    params.filterAnomalousOnly = true;
    auto anomalies = dataManager.queryData(params);
    
    if (!anomalies.empty()) {
        std::vector<SensorData> anomalyData;
        for (const auto& result : anomalies) {
            anomalyData.push_back(static_cast<SensorData>(result));
        }
        dataStorage.exportAnomaliesToJson(anomalyData);
        std::cout << "Exported " << anomalies.size() << " anomalies to anomaly_report.json" << std::endl;
    }
    
    return 0;
}

// Client mode function
int runClientMode(const std::string& serverIp, int serverPort) {
    std::cout << "Starting Smart Classroom Monitoring Client" << std::endl;
    std::cout << "Connecting to server at " << serverIp << ":" << serverPort << std::endl;
    
    Client client(serverIp, serverPort);
    
    if (!client.connectToServer(3, 1000)) {
        std::cerr << "Failed to connect to server. Exiting." << std::endl;
        return 1;
    }
    
    std::cout << "Connected! Sending sensor data every 2 seconds..." << std::endl;
    std::cout << "Press Ctrl+C to stop the client." << std::endl;
    
    // Run client for a demo period
    for (int i = 0; i < 10; ++i) {
        SensorData data = client.readSensorData();
        if (client.sendData(data)) {
            std::cout << "Sent: " << data.toString() << std::endl;
        } else {
            std::cerr << "Failed to send data" << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    
    client.disconnect();
    std::cout << "🔌 Client disconnected." << std::endl;
    return 0;
}

void printUsage(const char* programName) {
    std::cout << "Smart Classroom Monitoring System\n";
    std::cout << "====================================\n";
    std::cout << "Usage:\n";
    std::cout << "  " << programName << "                    - Interactive CLI mode\n";
    std::cout << "  " << programName << " server <port>      - Run as server\n";
    std::cout << "  " << programName << " client <ip> <port> - Run as client\n";
    std::cout << "\nExamples:\n";
    std::cout << "  " << programName << " server 8080\n";
    std::cout << "  " << programName << " client 127.0.0.1 8080\n";
}

int main(int argc, char* argv[]) {
    // Check for command line arguments for server/client modes
    if (argc > 1) {
        std::string mode = argv[1];
        
        if (mode == "server" && argc == 3) {
            int port = std::atoi(argv[2]);
            if (port <= 0 || port > 65535) {
                std::cerr << "Error: Invalid port number. Must be between 1 and 65535." << std::endl;
                return 1;
            }
            return runServerMode(port);
        }
        else if (mode == "client" && argc == 4) {
            std::string serverIp = argv[2];
            int port = std::atoi(argv[3]);
            if (port <= 0 || port > 65535) {
                std::cerr << "Error: Invalid port number. Must be between 1 and 65535." << std::endl;
                return 1;
            }
            return runClientMode(serverIp, port);
        }
        else {
            printUsage(argv[0]);
            return 1;
        }
    }
    
    // Default interactive CLI mode
    std::cout << "Smart Classroom Monitoring CLI (Part 4 - Query & Sync)\n";
    std::cout << "=========================================================\n";
    
    AnomalyDetector::AnomalyThresholds currentThresholds;
    DataManager dataManager(currentThresholds);

    auto now_ms = [] { return SensorData::time_point_to_ms(std::chrono::system_clock::now()); };
    dataManager.addSensorData({now_ms(), 22.0, 45.0, 500.0});         // Normal data
    dataManager.addSensorData({now_ms(), 12.0, 50.0, 300.0});         // Anomalous data (low temperature)
    dataManager.addSensorData({now_ms(), 25.0, 80.0, 250.0});         // Anomalous data (high humidity)
    dataManager.addSensorData({now_ms(), 35.0, 55.0, 50.0});          // Anomalous data (high temperature and low light intensity)
    dataManager.addSensorData({now_ms(), 26.0, 65.0, 1200.0});        // Anomalous data (high light intensity)

    std::string line;
    displayHelp();

    while (true) {
        std::cout << "cli> ";
        if (!std::getline(std::cin, line)) {
            break;
        }

        if (line.empty()) {
            continue;
        }

        std::stringstream ss(line);
        std::string command;
        ss >> command;

        if (command == "exit") {
            std::cout << "Exiting application." << std::endl;
            break;
        } else if (command == "help") {
            displayHelp();
        } else if (command == "add") {
            SensorData newData;
            if (!(ss >> newData.timestamp_ms >> newData.temperature >> newData.humidity >> newData.lightIntensity)) {
                std::cerr << "Error: Invalid 'add' command format. Please use: add <ts_ms> <temp> <hum> <light>\n";
                continue;
            }
            dataManager.addSensorData(newData);
            std::cout << "Sensor data added: " << newData.toString() << std::endl;

        } else if (command == "query") {
            DataManager::QueryParams queryParams;
            std::string token;
            bool proceed_with_query = true;

            while (ss >> token) {
                if (token == "anomalous") {
                    queryParams.filterAnomalousOnly = true;
                } else if (token == "normal") {
                    queryParams.filterAnomalousOnly = false;
                } else if (token == "sort") {
                    std::string criteriaStr;
                    if (!(ss >> criteriaStr)) {
                        std::cerr << "Error: Missing sort criteria after 'sort'. Query aborted.\n";
                        proceed_with_query = false;
                        break;
                    }
                    // Map input string to SortCriteria enum
                    if (criteriaStr == "ts_asc") queryParams.sortBy = SortCriteria::TIMESTAMP_ASC;
                    else if (criteriaStr == "ts_desc") queryParams.sortBy = SortCriteria::TIMESTAMP_DESC;
                    else if (criteriaStr == "temp_asc") queryParams.sortBy = SortCriteria::TEMP_ASC;
                    else if (criteriaStr == "temp_desc") queryParams.sortBy = SortCriteria::TEMP_DESC;
                    else if (criteriaStr == "hum_asc") queryParams.sortBy = SortCriteria::HUMIDITY_ASC;
                    else if (criteriaStr == "hum_desc") queryParams.sortBy = SortCriteria::HUMIDITY_DESC;
                    else if (criteriaStr == "light_asc") queryParams.sortBy = SortCriteria::LIGHT_ASC;
                    else if (criteriaStr == "light_desc") queryParams.sortBy = SortCriteria::LIGHT_DESC;
                    else if (criteriaStr == "dev_asc") queryParams.sortBy = SortCriteria::DEVIATION_ASC;
                    else if (criteriaStr == "dev_desc") queryParams.sortBy = SortCriteria::DEVIATION_DESC;
                    else {
                        std::cerr << "Error: Invalid sort criteria '" << criteriaStr << "'. Query aborted. Type 'help' for options.\n";
                        proceed_with_query = false;
                        break;
                    }
                } else {
                    std::cerr << "Error: Unknown token '" << token << "' in query command. Query aborted.\n";
                    proceed_with_query = false;
                    break;
                }
            }

            if (proceed_with_query) { 
                std::vector<QueryResult> results = dataManager.queryData(queryParams);
                printQueryResults(results);
            }

        } else {
            std::cerr << "Unknown command: '" << command << "'. Type 'help' for a list of commands.\n";
        }
    }

    std::cout << "Exiting program.\n";
    return 0;
}
