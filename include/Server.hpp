#pragma once
#include <thread>
#include <vector>
#include <atomic>

class Server {
public:
    Server(int port);
    ~Server();
    void start();
    void stop();
private:
    int server_fd;
    int port;
    std::atomic<bool> running;
    std::vector<std::thread> client_threads;
    void acceptClients();
    void handleClient(int client_socket);
};

