#include <gtest/gtest.h>
#include "Server.hpp"
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

// More tests can be added for edge cases, stress, etc.

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

