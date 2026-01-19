#ifndef UDP_INPUT_HPP
#define UDP_INPUT_HPP

#include "input.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <iostream>

class UdpInputProvider : public InputProvider {
public:
    UdpInputProvider(int port = 8888) : port_(port), sockfd_(-1), running_(false) {}

    ~UdpInputProvider() {
        if (sockfd_ >= 0) {
            close(sockfd_);
        }
    }

    bool init() {
        sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd_ < 0) {
            std::cerr << "Error: Failed to create socket" << std::endl;
            return false;
        }

        // Set socket to non-blocking with timeout
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 50000;  // 50ms timeout
        setsockopt(sockfd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        struct sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(port_);

        if (bind(sockfd_, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            std::cerr << "Error: Failed to bind socket to port " << port_ << std::endl;
            close(sockfd_);
            sockfd_ = -1;
            return false;
        }

        std::cout << "UDP input listening on port " << port_ << std::endl;
        running_ = true;
        return true;
    }

    bool read(InputState& state) override {
        if (!running_ || sockfd_ < 0) {
            return false;
        }

        char buffer[512];
        struct sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);

        ssize_t bytesReceived = recvfrom(sockfd_, buffer, sizeof(buffer) - 1, 0,
                                         (struct sockaddr*)&clientAddr, &clientAddrLen);

        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            std::cout << "Received: " << buffer << std::endl;  // Debug output
            parseInput(buffer, state);
        } else if (bytesReceived < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cerr << "recvfrom error: " << strerror(errno) << std::endl;
            state = lastState_;  // Keep last state on error
        } else {
            // Timeout - use last known state
            state = lastState_;
        }

        return running_;
    }

    void stop() {
        running_ = false;
    }

private:
    int port_;
    int sockfd_;
    bool running_;
    InputState lastState_ = {0, 0, 0, 0, false, false, false};  // Center joystick default

    void parseInput(const char* buffer, InputState& state) {
        // Parse Feather format: {"joyX":0,"joyY":0,"pot1":0,"pot2":0,"btn1":0,"btn2":0,"btn3":0}
        int x = 0, y = 0, p1 = 0, p2 = 0, b1 = 0, b2 = 0, b3 = 0;

        if (sscanf(buffer, "{\"joyX\":%d,\"joyY\":%d,\"pot1\":%d,\"pot2\":%d,\"btn1\":%d,\"btn2\":%d,\"btn3\":%d}",
                   &x, &y, &p1, &p2, &b1, &b2, &b3) >= 2) {
            state.joystick_x = x;
            state.joystick_y = y;
            state.potentiometer = p1;
            state.potentiometer2 = p2;
            state.button1 = (b1 != 0);
            state.button2 = (b2 != 0);
            state.button3 = (b3 != 0);
            lastState_ = state;
        } else {
            state = lastState_;
        }
    }
};

#endif
