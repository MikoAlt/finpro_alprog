#include "gtest/gtest.h"
#include "Client.hpp"
#include "SensorData.hpp"

#include <thread>
#include <atomic>
#include <vector>
#include <chrono>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "Ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    typedef int SOCKET;
    inline int closesocket(SOCKET s) { return close(s); }
#endif

class MockServer {
public:
    std::atomic<bool> running{false};
    std::atomic<bool> connection_accepted{false};
    std::atomic<int> data_bytes_received{0};
    SOCKET listen_socket = INVALID_SOCKET;
    SOCKET client_socket = INVALID_SOCKET;
    int port;
    std::thread server_thread;
    std::string last_received_data;

    MockServer(int p) : port(p) {}

    void start() {
        running = true;
        connection_accepted = false;
        data_bytes_received = 0;
        last_received_data.clear();

        server_thread = std::thread([this]() {
#ifdef _WIN32
            WSADATA wsaData;
            if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
                std::cerr << "MockServer: WSAStartup failed.\n";
                return;
            }
#endif
            listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (listen_socket == INVALID_SOCKET) {
                std::cerr << "MockServer: Socket creation failed.\n";
                running = false;
#ifdef _WIN32
                WSACleanup();
#endif
                return;
            }

            sockaddr_in server_addr;
            server_addr.sin_family = AF_INET;
            server_addr.sin_addr.s_addr = INADDR_ANY;
            server_addr.sin_port = htons(port);

            if (bind(listen_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
                std::cerr << "MockServer: Bind failed.\n";
                closesocket(listen_socket);
                listen_socket = INVALID_SOCKET;
                running = false;
#ifdef _WIN32
                WSACleanup();
#endif
                return;
            }

            if (listen(listen_socket, 1) == SOCKET_ERROR) {
                std::cerr << "MockServer: Listen failed.\n";
                closesocket(listen_socket);
                listen_socket = INVALID_SOCKET;
                running = false;
#ifdef _WIN32
                WSACleanup();
#endif
                return;
            }

            fd_set read_fds;
            struct timeval tv;
            tv.tv_sec = 1;
            tv.tv_usec = 0;

            while (running) {
                FD_ZERO(&read_fds);
                FD_SET(listen_socket, &read_fds);

                int activity = select(listen_socket + 1, &read_fds, nullptr, nullptr, &tv);

                if (activity < 0 && running) {
                    std::cerr << "MockServer: Select error on listen socket.\n";
                    break;
                }

                if (activity > 0 && FD_ISSET(listen_socket, &read_fds)) {
                    client_socket = accept(listen_socket, nullptr, nullptr);
                    if (client_socket == INVALID_SOCKET) {
                        if(running) std::cerr << "MockServer: Accept failed.\n";
                        break;
                    }
                    connection_accepted = true;
                    char buffer[1024];
                    int bytes_in;
                    while(running && connection_accepted && (bytes_in = recv(client_socket, buffer, sizeof(buffer) - 1, 0)) > 0) {
                        buffer[bytes_in] = '\0';
                        last_received_data.append(buffer, bytes_in);
                        data_bytes_received += bytes_in;
                    }
                    if (client_socket != INVALID_SOCKET) {
                        closesocket(client_socket);
                        client_socket = INVALID_SOCKET;
                    }
                    connection_accepted = false;
                }
            }

            if (listen_socket != INVALID_SOCKET) {
                closesocket(listen_socket);
                listen_socket = INVALID_SOCKET;
            }
#ifdef _WIN32
            WSACleanup();
#endif
        });
    }

    void stop() {
        running = false;
        if (listen_socket != INVALID_SOCKET) {
            closesocket(listen_socket);
            listen_socket = INVALID_SOCKET;
        }
        if (client_socket != INVALID_SOCKET) {
            closesocket(client_socket);
            client_socket = INVALID_SOCKET;
        }
        if (server_thread.joinable()) {
            server_thread.join();
        }
    }

    ~MockServer() {
        stop();
    }
};

class ClientTest : public ::testing::Test {
protected:
    const char* test_server_ip = "127.0.0.1";
    const int test_server_port = 9999;
    std::unique_ptr<MockServer> mock_server;

    void SetUp() override {
    }

    void TearDown() override {
        if (mock_server) {
            mock_server->stop();
            mock_server.reset();
        }
    }

    void startMockServer() {
        mock_server = std::make_unique<MockServer>(test_server_port);
        mock_server->start();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
};


TEST_F(ClientTest, ReadSensorDataReturnsValidStructure) {
    Client client("dummy_ip", 0);
    SensorData data = client.readSensorData();

    ASSERT_GE(data.temperature, 18.0);
    ASSERT_LE(data.temperature, 30.0);
    ASSERT_GE(data.humidity, 30.0);
    ASSERT_LE(data.humidity, 70.0);
    ASSERT_GE(data.lightIntensity, 100.0);
    ASSERT_LE(data.lightIntensity, 1000.0);
    ASSERT_GT(data.timestamp, 0);
}

TEST_F(ClientTest, ConnectToServerSuccessfully) {
    startMockServer();
    ASSERT_TRUE(mock_server && mock_server->running);

    Client client(test_server_ip, test_server_port);
    ASSERT_TRUE(client.connectToServer(1, 100));

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    ASSERT_TRUE(mock_server->connection_accepted);
    client.disconnect();
}

TEST_F(ClientTest, ConnectToServerFailsIfServerNotRunning) {
    Client client(test_server_ip, test_server_port);
    ASSERT_FALSE(client.connectToServer(2, 50));
}

TEST_F(ClientTest, SendDataSuccessfullyAfterConnection) {
    startMockServer();
    ASSERT_TRUE(mock_server && mock_server->running);

    Client client(test_server_ip, test_server_port);
    ASSERT_TRUE(client.connectToServer(1,100));

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    ASSERT_TRUE(mock_server->connection_accepted);

    SensorData test_data = {25.5, 55.1, 500.0, 1234567890LL};
    std::string expected_data_str = test_data.toString() + "\n";

    ASSERT_TRUE(client.sendData(test_data));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    ASSERT_GT(mock_server->data_bytes_received, 0);
    ASSERT_EQ(mock_server->last_received_data, expected_data_str);
    client.disconnect();
}

TEST_F(ClientTest, SendDataAttemptsReconnectIfInitiallyDisconnected) {
    startMockServer();
    Client client(test_server_ip, test_server_port);
    ASSERT_TRUE(client.connectToServer(1,100));
    SensorData data1 = {20.0, 40.0, 200.0, 1000LL};
    ASSERT_TRUE(client.sendData(data1));
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    ASSERT_GT(mock_server->data_bytes_received, 0);
    mock_server->last_received_data.clear();
    mock_server->data_bytes_received = 0;

    client.disconnect();

    mock_server->stop();
    mock_server.reset();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    startMockServer();

    SensorData data2 = {22.0, 45.0, 250.0, 2000LL};
    std::string expected_data_str = data2.toString() + "\n";

    ASSERT_TRUE(client.sendData(data2));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ASSERT_TRUE(mock_server->connection_accepted);
    ASSERT_GT(mock_server->data_bytes_received, 0);
    ASSERT_EQ(mock_server->last_received_data, expected_data_str);

    client.disconnect();
}

TEST_F(ClientTest, SendDataFailsIfReconnectFails) {
    Client client(test_server_ip, test_server_port);
    SensorData test_data = {25.5, 55.1, 500.0, 1234567890LL};
    ASSERT_FALSE(client.sendData(test_data));
}