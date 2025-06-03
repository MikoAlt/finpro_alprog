//
// Created by Miko on 5/21/2025.
//

#include "main.hpp"
#include "Server.hpp"

int main() {
    Server server(8080); // Port 8080, can be changed as needed
    server.start();
    // Keep the server running
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return 0;
}
