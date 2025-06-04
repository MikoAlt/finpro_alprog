#include <gtest/gtest.h>
#include "Server.hpp"
#include "DataManager.hpp"
#include "DataStorage.hpp"
#include "SensorData.hpp"
#include <thread>
#include <chrono>
#include <atomic>
#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

// Helper function to connect as a client and send a message
void client_send_message(const char* message, int port) {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);
#endif
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_NE(sock, -1);
    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    ASSERT_EQ(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)), 0);
    send(sock, message, strlen(message), 0);
#ifdef _WIN32
    closesocket(sock);
    WSACleanup();
#else
    close(sock);
#endif
}

TEST(ServerTest, AcceptsMultipleClients) {
    int port = 9090;
    Server server(port);
    server.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200)); // Wait for server to start
    std::thread t1(client_send_message, "Hello1", port);
    std::thread t2(client_send_message, "Hello2", port);
    t1.join();
    t2.join();
    // If no crash, test passes (for real test, capture output or add hooks)
    server.stop();
}

TEST(ServerTest, HandlesClientCommunication) {
    int port = 9091;
    Server server(port);
    server.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    std::thread t([&](){ client_send_message("TestMessage", port); });
    t.join();
    server.stop();
}

// Integration test for Server-DataManager-DataStorage communication
TEST(ServerTest, IntegratesWithDataManagerAndStorage) {
    int port = 9092;
    
    // Set up DataManager and DataStorage
    AnomalyDetector::AnomalyThresholds thresholds;
    thresholds.minTemp = 15.0;
    thresholds.maxTemp = 30.0;
    thresholds.minHumidity = 30.0;
    thresholds.maxHumidity = 70.0;
    thresholds.minLight = 100.0;
    thresholds.maxLight = 1000.0;
    
    DataManager dataManager(thresholds);
    DataStorage dataStorage("test_sensor_data.bin", "test_anomalies.json");
    
    // Create server with DataManager and DataStorage integration
    Server server(port, &dataManager, &dataStorage);
    
    // Set up callback to verify data processing
    std::atomic<bool> dataReceived{false};
    SensorData receivedData;
    server.setDataCallback([&](const SensorData& data) {
        receivedData = data;
        dataReceived = true;
    });
    
    server.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200)); // Wait for server to start
    
    // Create sensor data in the expected format
    auto now_ms = SensorData::time_point_to_ms(std::chrono::system_clock::now());
    SensorData testData{now_ms, 25.5, 60.0, 750.0};
    std::string dataStr = testData.toString();
    
    // Send the properly formatted sensor data
    std::thread clientThread([&]() {
        client_send_message(dataStr.c_str(), port);
    });
    
    clientThread.join();
    
    // Give some time for data processing
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    server.stop();
    
    // Verify that the callback was called
    EXPECT_TRUE(dataReceived.load());
    
    // Verify that data was added to DataManager
    DataManager::QueryParams params;
    auto results = dataManager.queryData(params);
    EXPECT_GT(results.size(), 0);
    
    if (!results.empty()) {
        EXPECT_DOUBLE_EQ(results[0].temperature, 25.5);
        EXPECT_DOUBLE_EQ(results[0].humidity, 60.0);
        EXPECT_DOUBLE_EQ(results[0].lightIntensity, 750.0);
        EXPECT_FALSE(results[0].isAnomalousFlag); // Should be normal data
    }
}

// Test Server-Client integration with real sensor data format
TEST(ServerTest, ParsesRealSensorDataFormat) {
    int port = 9093;
    DataManager dataManager(AnomalyDetector::AnomalyThresholds{});
    Server server(port, &dataManager, nullptr);
    
    std::atomic<int> dataCount{0};
    server.setDataCallback([&](const SensorData& data) {
        dataCount++;
    });
    
    server.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Send multiple sensor readings in correct format
    std::vector<std::string> testMessages = {
        "Timestamp (ms): 1640995200000, Temp: 22.50 C, Humidity: 45.30 %, Light: 500.00 lux",
        "Timestamp (ms): 1640995260000, Temp: 35.00 C, Humidity: 80.00 %, Light: 50.00 lux", // Anomalous
        "Timestamp (ms): 1640995320000, Temp: 20.00 C, Humidity: 55.00 %, Light: 800.00 lux"  // Normal
    };
    
    std::vector<std::thread> clientThreads;
    for (const auto& msg : testMessages) {
        clientThreads.emplace_back([&, msg]() {
            client_send_message(msg.c_str(), port);
        });
    }
    
    for (auto& t : clientThreads) {
        t.join();
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    server.stop();
    
    // Verify all data was processed
    EXPECT_EQ(dataCount.load(), 3);
    
    // Query the data to verify it was stored correctly
    DataManager::QueryParams params;
    auto results = dataManager.queryData(params);
    EXPECT_EQ(results.size(), 3);
    
    // Check that anomaly detection worked
    params.filterAnomalousOnly = true;
    auto anomalies = dataManager.queryData(params);
    EXPECT_GT(anomalies.size(), 0); // Should have at least one anomaly
}

// More tests can be added for edge cases, stress, etc.

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

