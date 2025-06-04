#pragma once
#include <thread>
#include <vector>
#include <atomic>
#include <functional>
#include "SensorData.hpp"
#include "DataManager.hpp"
#include "DataStorage.hpp"

class Server {
public:
    Server(int port);
    Server(int port, DataManager* dataManager, DataStorage* dataStorage);
    ~Server();
    void start();
    void stop();
    
    // Callback for when data is received
    void setDataCallback(std::function<void(const SensorData&)> callback);
    
private:
    int server_fd;
    int port;
    std::atomic<bool> running;
    std::vector<std::thread> client_threads;
    
    // Data processing components
    DataManager* dataManager_;
    DataStorage* dataStorage_;
    std::function<void(const SensorData&)> dataCallback_;
    
    void acceptClients();
    void handleClient(int client_socket);
    void processReceivedData(const std::string& dataStr);
};

