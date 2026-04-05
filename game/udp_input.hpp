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
#include <cmath>
#include <cstdlib>

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
            // std::cout << "Received: " << buffer << std::endl;  // Debug output
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
    InputState lastState_ = {0, 0, 0, 0, false, false, false, false};  // Center joystick default
    int lockedRatio_ = 50;    // Locked shield/firerate ratio (0-100)
    int lastLight_ = 0;       // Previous light reading for change detection
    static constexpr int LIGHT_CHANGE_THRESHOLD = 500;  // Light change needed to lock ratio

    // Tilt configuration
    static constexpr float TILT_DEADZONE = 1.0f;    // Ignore tilts below this (m/s^2)
    static constexpr float TILT_MAX = 6.0f;         // Full tilt at this value (m/s^2)

    // Helper to find a float value in JSON by key
    float parseJsonFloat(const char* buffer, const char* key) {
        char searchKey[32];
        snprintf(searchKey, sizeof(searchKey), "\"%s\":", key);
        const char* pos = strstr(buffer, searchKey);
        if (pos) {
            pos += strlen(searchKey);
            return atof(pos);
        }
        return 0.0f;
    }

    // Helper to find an int value in JSON by key
    int parseJsonInt(const char* buffer, const char* key) {
        char searchKey[32];
        snprintf(searchKey, sizeof(searchKey), "\"%s\":", key);
        const char* pos = strstr(buffer, searchKey);
        if (pos) {
            pos += strlen(searchKey);
            return atoi(pos);
        }
        return 0;
    }

    // Convert accelerometer tilt to joystick value (-100 to 100)
    int tiltToJoystick(float accel) {
        // Apply deadzone
        if (fabs(accel) < TILT_DEADZONE) {
            return 0;
        }

        // Remove deadzone from calculation
        float sign = (accel > 0) ? 1.0f : -1.0f;
        float adjusted = fabs(accel) - TILT_DEADZONE;
        float range = TILT_MAX - TILT_DEADZONE;

        // Map to -100 to 100
        int value = (int)(sign * (adjusted / range) * 100.0f);

        // Clamp to range
        if (value > 100) value = 100;
        if (value < -100) value = -100;

        return value;
    }

    void parseInput(const char* buffer, InputState& state) {
        // Check if this is the new I2C controller format (has accelX)
        if (strstr(buffer, "\"accelX\"") != nullptr) {
            // New I2C controller format with IMU
            // {"btn1":0,"btn2":0,"btn3":0,"btn4":0,"prox":123,"light":456,"accelX":0.12,"accelY":-0.34,"accelZ":9.81,"gyroX":0.001,"gyroY":-0.002,"gyroZ":0.000,"temp":25.3}

            float accelX = parseJsonFloat(buffer, "accelX");
            float accelY = parseJsonFloat(buffer, "accelY");
            int prox = parseJsonInt(buffer, "prox");
            int light = parseJsonInt(buffer, "light");
            int b1 = parseJsonInt(buffer, "btn1");
            int b2 = parseJsonInt(buffer, "btn2");
            int b3 = parseJsonInt(buffer, "btn3");
            int b4 = parseJsonInt(buffer, "btn4");

            // Convert tilt to joystick movement
            // accelX controls left/right, accelY controls up/down
            // Note: You may need to swap or invert axes depending on sensor orientation
            state.joystick_x = tiltToJoystick(accelX);
            state.joystick_y = tiltToJoystick(-accelY);  // Inverted for natural feel

            // Shield/firerate ratio: proximity sets it, sudden light change locks it
            // Map proximity (0-65535 raw -> 0-100)
            int currentProxRatio = (prox > 6553) ? 100 : (prox * 100 / 6553);

            // Check for sudden light change to lock the ratio
            int lightDelta = abs(light - lastLight_);
            if (lightDelta > LIGHT_CHANGE_THRESHOLD) {
                // Lock in the current proximity as the ratio
                lockedRatio_ = currentProxRatio;
                std::cout << "[INPUT] Ratio locked at " << lockedRatio_ << "% (light delta: " << lightDelta << ")" << std::endl;
            }
            lastLight_ = light;

            // Use locked ratio as shield/firerate
            state.potentiometer = lockedRatio_;

            // potentiometer2 shows current (unlocked) proximity for feedback
            state.potentiometer2 = currentProxRatio;

            state.button1 = (b1 != 0);
            state.button2 = (b2 != 0);
            state.button3 = (b3 != 0);
            state.button4 = (b4 != 0);  // Weapon toggle

            lastState_ = state;
        }
        // Legacy joystick format
        else if (strstr(buffer, "\"joyX\"") != nullptr) {
            // Original Feather format: {"joyX":0,"joyY":0,"pot1":0,"pot2":0,"btn1":0,"btn2":0,"btn3":0}
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
                state.button4 = false;  // Legacy controller doesn't have btn4
                lastState_ = state;
            } else {
                state = lastState_;
            }
        }
        else {
            state = lastState_;
        }
    }
};

#endif
