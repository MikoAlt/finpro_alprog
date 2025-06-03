#include "SensorData.hpp"        
#include "AnomalyDetector.hpp"   
#include "DataManager.hpp"       
#include "QueryCommon.hpp"       


#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <iomanip>               // For std::setw, std::fixed, std::setprecision
#include <chrono>                // For std::chrono::system_clock for demo data
// #include <thread>             // For std::this_thread::sleep_for  (optional, for demo purposes)

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
    std::cout << std::string(90, '-') << std::endl; // Separator line

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

// Function to display help instructions for the CLI
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

int main(int argc, char* argv[]) {
    AnomalyDetector::AnomalyThresholds currentThresholds;
    DataManager dataManager(currentThresholds);

    auto now_ms = [] { return SensorData::time_point_to_ms(std::chrono::system_clock::now()); };
    dataManager.addSensorData({now_ms(), 22.0, 45.0, 500.0});         // Normal data

    // std::this_thread::sleep_for(std::chrono::milliseconds(20));    // Optional pause

    dataManager.addSensorData({now_ms(), 12.0, 50.0, 300.0});         // Anomalous data (low temperature)

    // std::this_thread::sleep_for(std::chrono::milliseconds(20));    // Optional pause

    dataManager.addSensorData({now_ms(), 25.0, 80.0, 250.0});         // Anomalous data (high humidity)

    // std::this_thread::sleep_for(std::chrono::milliseconds(20));    // Optional pause

    dataManager.addSensorData({now_ms(), 35.0, 55.0, 50.0});          // Anomalous data (high temperature and low light intensity)

    // std::this_thread::sleep_for(std::chrono::milliseconds(20));    // Optional pause

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
                continue; // Skip to next command prompt
            }
            dataManager.addSensorData(newData);
            std::cout << "Sensor data added: " << newData.toString() << std::endl;

        } else if (command == "query") {
            DataManager::QueryParams queryParams; // Default sort is TIMESTAMP_ASC
            std::string token;
            bool proceed_with_query = true; // Flag to control if query proceeds

            // Inner loop to parse all arguments for the "query" command
            while (ss >> token) {
                if (token == "anomalous") {
                    queryParams.filterAnomalousOnly = true;
                } else if (token == "normal") {
                    queryParams.filterAnomalousOnly = false;
                } else if (token == "sort") {
                    std::string criteriaStr;
                    if (!(ss >> criteriaStr)) { // Check if a criteria string follows "sort"
                        std::cerr << "Error: Missing sort criteria after 'sort'. Query aborted.\n";
                        proceed_with_query = false; // Mark that query should not proceed
                        break; // Exit this inner token parsing loop
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
                        proceed_with_query = false; // Mark that query should not proceed
                        break; // Exit this inner token parsing loop
                    }
                } else { // Unknown token
                    std::cerr << "Error: Unknown token '" << token << "' in query command. Query aborted.\n";
                    proceed_with_query = false; // Mark that query should not proceed
                    break; // Exit this inner token parsing loop
                }
            } // End of inner while loop for parsing query arguments

            // Only execute the query if all arguments were parsed successfully
            if (proceed_with_query) { 
                std::vector<QueryResult> results = dataManager.queryData(queryParams);
                printQueryResults(results);
            }
            // If proceed_with_query is false, an error message was already printed.


        } else {
            std::cerr << "Unknown command: '" << command << "'. Type 'help' for a list of commands.\n";
        }
    } // End of main while (true) loop

    std::cout << "Exiting program.\n"; // Message when exiting via "exit" or EOF
    return 0;
}