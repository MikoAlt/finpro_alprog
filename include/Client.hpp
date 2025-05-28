#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include "SensorData.hpp"

class Client {
public:
    Client(const std::string& server_ip, int server_port);
    ~Client();
    SensorData readSensorData();
    bool connectToServer(int max_retries = 10, int retry_delay_ms = 1000);
    bool sendData(const SensorData& data);
    void disconnect();

private:
    std::string server_ip_;
    int server_port_;
    int sock_;
    bool connected_;
    void initializeSocketLib();
    void cleanupSocketLib();
};

#endif // CLIENT_HPP