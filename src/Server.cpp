#include "../include/Server.hpp"
#include <iostream>
#include <cstring>
#include <sstream>
#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#endif

Server::Server(int port) : port(port), running(false), server_fd(-1), dataManager_(nullptr), dataStorage_(nullptr) {}

Server::Server(int port, DataManager* dataManager, DataStorage* dataStorage) 
    : port(port), running(false), server_fd(-1), dataManager_(dataManager), dataStorage_(dataStorage) {}

Server::~Server() {
    stop();
}

void Server::setDataCallback(std::function<void(const SensorData&)> callback) {
    dataCallback_ = callback;
}

void Server::start() {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);
#endif
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Socket creation failed!" << std::endl;
        return;
    }
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed!" << std::endl;
        return;
    }
    if (listen(server_fd, 5) < 0) {
        std::cerr << "Listen failed!" << std::endl;
        return;
    }
    running = true;
    std::thread(&Server::acceptClients, this).detach();
    std::cout << "Server started on port " << port << std::endl;
}

void Server::stop() {
    running = false;
#ifdef _WIN32
    closesocket(server_fd);
    WSACleanup();
#else
    close(server_fd);
#endif
    for (auto& t : client_threads) {
        if (t.joinable()) t.join();
    }
}

void Server::acceptClients() {
    while (running) {
        sockaddr_in client_addr{};
#ifdef _WIN32
        int addrlen = sizeof(client_addr);
#else
        socklen_t addrlen = sizeof(client_addr);
#endif
        int client_socket = accept(server_fd, (struct sockaddr*)&client_addr, &addrlen);
        if (client_socket < 0) continue;
        client_threads.emplace_back(&Server::handleClient, this, client_socket);
    }
}

void Server::handleClient(int client_socket) {
    char buffer[1024];
    while (running) {
        int bytes = recv(client_socket, buffer, sizeof(buffer)-1, 0);
        if (bytes <= 0) break;
        buffer[bytes] = '\0';
        
        std::string receivedData(buffer);
        std::cout << "Received: " << receivedData << std::endl;
        
        // Process the received sensor data
        processReceivedData(receivedData);
        
        // Send acknowledgment back to client
        const char* ack = "ACK\n";
        send(client_socket, ack, strlen(ack), 0);
    }
#ifdef _WIN32
    closesocket(client_socket);
#else
    close(client_socket);
#endif
}

void Server::processReceivedData(const std::string& dataStr) {
    try {
        // Parse the received data into SensorData structure
        SensorData sensorData = SensorData::fromString(dataStr);
        
        // If timestamp is 0, it means parsing failed
        if (sensorData.timestamp_ms == 0) {
            std::cerr << "Failed to parse sensor data: " << dataStr << std::endl;
            return;
        }
        
        // Call the registered callback if available
        if (dataCallback_) {
            dataCallback_(sensorData);
        }
        
        // Store data using DataManager if available
        if (dataManager_) {
            dataManager_->addSensorData(sensorData);
        }
        
        // Store data using DataStorage if available
        if (dataStorage_) {
            dataStorage_->storeData(sensorData);
        }
        
        std::cout << "Processed sensor data: " << sensorData.toString() << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error processing received data: " << e.what() << std::endl;
    }
}

