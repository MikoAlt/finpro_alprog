#include "client.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <sstream>
#include <iomanip>
#include <random>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "Ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <netdb.h>
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    typedef int SOCKET;
    inline int closesocket(SOCKET s) { return close(s); }
#endif

SensorData SensorData::fromString(const std::string& dataStr) {
    SensorData data = {0, 0.0, 0.0, 0.0};
    
    // Parse format: "Timestamp (ms): 1640995200000, Temp: 22.50 C, Humidity: 45.30 %, Light: 500.00 lux"
    std::istringstream iss(dataStr);
    std::string token;
    
    try {
        // Parse timestamp
        while (iss >> token && token != "Timestamp") {}
        iss >> token; // "(ms):"
        iss >> data.timestamp_ms;
        
        // Parse temperature
        while (iss >> token && token != "Temp:") {}
        iss >> data.temperature;
        iss >> token; // "C,"
        
        // Parse humidity
        while (iss >> token && token != "Humidity:") {}
        iss >> data.humidity;
        iss >> token; // "%,"
        
        // Parse light intensity
        while (iss >> token && token != "Light:") {}
        iss >> data.lightIntensity;
        
    } catch (const std::exception& e) {
        std::cerr << "Error parsing sensor data: " << e.what() << std::endl;
        data = {0, 0.0, 0.0, 0.0}; // Reset to default values on error
    }
    
    return data;
}

Client::Client(const std::string& server_ip, int server_port)
    : server_ip_(server_ip), server_port_(server_port), sock_(INVALID_SOCKET), connected_(false) {
    std::random_device rd;
    rng_ = std::mt19937(rd());
    temp_dist_ = std::uniform_real_distribution<double>(18.0, 30.0);
    hum_dist_ = std::uniform_real_distribution<double>(30.0, 70.0);
    light_dist_ = std::uniform_real_distribution<double>(100.0, 1000.0);

    initializeSocketLib();
}

Client::~Client() {
    disconnect();
    cleanupSocketLib();
}

void Client::initializeSocketLib() {
#ifdef _WIN32
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
    }
#endif
}

void Client::cleanupSocketLib() {
#ifdef _WIN32
    WSACleanup();
#endif
}

SensorData Client::readSensorData() {
    SensorData data;
    data.temperature = temp_dist_(rng_);
    data.humidity = hum_dist_(rng_);
    data.lightIntensity = light_dist_(rng_);
    data.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::system_clock::now().time_since_epoch())
                         .count();
    return data;
}

bool Client::connectToServer(int max_retries, int retry_delay_ms) {
    if (connected_) {
        std::cout << "Already connected." << std::endl;
        return true;
    }

    for (int attempt = 0; attempt < max_retries; ++attempt) {
        sock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock_ == INVALID_SOCKET) {
            std::cerr << "Socket creation failed. Error: "
#ifdef _WIN32
                      << WSAGetLastError()
#else
                      << errno
#endif
                      << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay_ms));
            continue;
        }

        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(server_port_);

#ifdef _WIN32
        if (inet_pton(AF_INET, server_ip_.c_str(), &server_addr.sin_addr) <= 0) {
            struct hostent *host = gethostbyname(server_ip_.c_str());
            if (host == nullptr || host->h_addr_list[0] == nullptr) {
                std::cerr << "Invalid address/ Address not supported / Hostname resolution failed." << std::endl;
                closesocket(sock_);
                sock_ = INVALID_SOCKET;
                std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay_ms));
                continue;
            }
            memcpy(&server_addr.sin_addr, host->h_addr_list[0], host->h_length);
        }
#else
        if (inet_pton(AF_INET, server_ip_.c_str(), &server_addr.sin_addr) <= 0) {
            struct hostent *host = gethostbyname(server_ip_.c_str());
            if (host == nullptr || host->h_addr_list[0] == nullptr) {
                std::cerr << "Invalid address/ Address not supported / Hostname resolution failed." << std::endl;
                close(sock_);
                sock_ = INVALID_SOCKET;
                std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay_ms));
                continue;
            }
            memcpy(&server_addr.sin_addr, host->h_addr_list[0], host->h_length);
        }
#endif

        if (connect(sock_, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
            std::cerr << "Connection attempt " << (attempt + 1) << " failed. Error: "
#ifdef _WIN32
                      << WSAGetLastError()
#else
                      << errno
#endif
                      << std::endl;
            closesocket(sock_);
            sock_ = INVALID_SOCKET;
            if (attempt < max_retries - 1) {
                std::cout << "Retrying in " << retry_delay_ms / 1000.0 << " seconds..." << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay_ms));
            }
        } else {
            std::cout << "Successfully connected to server " << server_ip_ << ":" << server_port_ << std::endl;
            connected_ = true;
            return true;
        }
    }
    std::cerr << "Failed to connect to server after " << max_retries << " attempts." << std::endl;
    return false;
}

bool Client::sendData(const SensorData& data) {
    if (!connected_ || sock_ == INVALID_SOCKET) {
        std::cerr << "Not connected to server. Cannot send data." << std::endl;
        std::cout << "Attempting to reconnect..." << std::endl;
        if (!connectToServer(1, 0)) {
            return false;
        }
    }

    std::string data_str = data.toString();
    data_str += "\n";

    int bytes_sent = send(sock_, data_str.c_str(), static_cast<int>(data_str.length()), 0);
    if (bytes_sent == SOCKET_ERROR) {
        std::cerr << "Send failed. Error: "
#ifdef _WIN32
                  << WSAGetLastError()
#else
                  << errno
#endif
                  << std::endl;
        disconnect();
        return false;
    }
    if (bytes_sent < static_cast<int>(data_str.length())) {
        std::cerr << "Warning: Not all data was sent. Sent " << bytes_sent << " of " << data_str.length() << std::endl;
    }
    return true;
}

std::string Client::receiveResponse() {
    if (!connected_ || sock_ == INVALID_SOCKET) {
        std::cerr << "Not connected to server. Cannot receive data." << std::endl;
        return "";
    }
    char buffer[1024] = {0};
    int bytes_received = recv(sock_, buffer, sizeof(buffer) - 1, 0);

    if (bytes_received == SOCKET_ERROR) {
        std::cerr << "Receive failed. Error: "
#ifdef _WIN32
                  << WSAGetLastError()
#else
                  << errno
#endif
                  << std::endl;
        disconnect();
        return "";
    } else if (bytes_received == 0) {
        std::cout << "Server closed the connection." << std::endl;
        disconnect();
        return "";
    }
    buffer[bytes_received] = '\0';
    return std::string(buffer);
}

void Client::disconnect() {
    if (sock_ != INVALID_SOCKET) {
        closesocket(sock_);
        sock_ = INVALID_SOCKET;
    }
    connected_ = false;
}

void Client::run() {
    const int SEND_INTERVAL_SECONDS = 5;

    while (true) {
        if (!connected_) {
            std::cout << "Client not connected. Attempting to connect..." << std::endl;
            if (!connectToServer()) {
                std::cout << "Failed to connect. Will retry later." << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(SEND_INTERVAL_SECONDS * 2));
                continue;
            }
        }

        SensorData current_data = readSensorData();
        std::cout << "Read sensor data: " << current_data.toString() << std::endl;

        if (sendData(current_data)) {
            std::cout << "Data successfully sent to server." << std::endl;
        } else {
            std::cerr << "Failed to send data." << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::seconds(SEND_INTERVAL_SECONDS));
    }
}

