/*
 * Raspberry Pi UDP Receiver for Feather Input Controller
 *
 * Receives JSON data from Adafruit Feather M0 WiFi over UDP
 * and prints it to console for testing/debugging
 *
 * Compile: g++ -o pi_receiver pi_receiver.cpp
 * Run: ./pi_receiver
 */

#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

// Configuration
const int UDP_PORT = 8888;           // Must match the port in Feather code
const int BUFFER_SIZE = 512;         // Buffer for incoming data

int main() {
    std::cout << "=== Raspberry Pi UDP Receiver ===" << std::endl;
    std::cout << "Listening on port " << UDP_PORT << std::endl;
    std::cout << "Waiting for data from Feather...\n" << std::endl;

    // Create UDP socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "Error: Failed to create socket" << std::endl;
        return 1;
    }

    // Configure socket address
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;  // Listen on all interfaces
    serverAddr.sin_port = htons(UDP_PORT);

    // Bind socket to port
    if (bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error: Failed to bind socket to port " << UDP_PORT << std::endl;
        close(sockfd);
        return 1;
    }

    std::cout << "Socket bound successfully. Ready to receive data." << std::endl;
    std::cout << "Press Ctrl+C to exit.\n" << std::endl;

    // Buffer for incoming data
    char buffer[BUFFER_SIZE];
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);

    // Main receive loop
    while (true) {
        // Clear buffer
        memset(buffer, 0, BUFFER_SIZE);

        // Receive data
        ssize_t bytesReceived = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0,
                                         (struct sockaddr*)&clientAddr, &clientAddrLen);

        if (bytesReceived < 0) {
            std::cerr << "Error receiving data" << std::endl;
            continue;
        }

        // Null-terminate the received data
        buffer[bytesReceived] = '\0';

        // Get sender's IP address
        char senderIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddr.sin_addr), senderIP, INET_ADDRSTRLEN);

        // Print received data with timestamp
        std::cout << "[" << senderIP << "] " << buffer << std::endl;
    }

    // Cleanup (won't be reached unless we add signal handling)
    close(sockfd);
    return 0;
}
