#ifndef SERIAL_INPUT_HPP
#define SERIAL_INPUT_HPP

#include "input.hpp"
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <iostream>
#include <cmath>
#include <cstdlib>

class SerialInputProvider : public InputProvider {
public:
    SerialInputProvider(const char* device = "/dev/ttyACM0", int baud = 115200)
        : device_(device), baud_(baud), fd_(-1) {}

    ~SerialInputProvider() {
        if (fd_ >= 0) close(fd_);
    }

    bool init() {
        fd_ = open(device_, O_RDONLY | O_NOCTTY | O_NONBLOCK);
        if (fd_ < 0) {
            std::cerr << "[SERIAL] Failed to open " << device_ << ": " << strerror(errno) << std::endl;
            return false;
        }

        struct termios tty;
        if (tcgetattr(fd_, &tty) != 0) {
            std::cerr << "[SERIAL] tcgetattr failed: " << strerror(errno) << std::endl;
            close(fd_);
            fd_ = -1;
            return false;
        }

        // Raw mode, no echo, no signals
        cfmakeraw(&tty);
        tty.c_cflag |= (CLOCAL | CREAD);
        tty.c_cflag &= ~CSIZE;
        tty.c_cflag |= CS8;
        tty.c_cflag &= ~PARENB;
        tty.c_cflag &= ~CSTOPB;
        tty.c_cflag &= ~HUPCL;    // Don't drop DTR on close — prevents Arduino reset
        tty.c_cflag &= ~CRTSCTS;  // No hardware flow control
        tty.c_cc[VMIN]  = 0;
        tty.c_cc[VTIME] = 0;  // Non-blocking

        speed_t speed = B115200;
        cfsetispeed(&tty, speed);
        cfsetospeed(&tty, speed);

        if (tcsetattr(fd_, TCSANOW, &tty) != 0) {
            std::cerr << "[SERIAL] tcsetattr failed: " << strerror(errno) << std::endl;
            close(fd_);
            fd_ = -1;
            return false;
        }

        // Flush any boot/reset garbage before the game loop starts reading
        tcflush(fd_, TCIFLUSH);

        std::cout << "[SERIAL] Opened " << device_ << " at 115200 baud" << std::endl;
        return true;
    }

    bool read(InputState& state) override {
        if (fd_ < 0) return false;

        // Read available bytes into line buffer
        char chunk[128];
        ssize_t n = ::read(fd_, chunk, sizeof(chunk));
        if (n > 0) {
            for (ssize_t i = 0; i < n; i++) {
                char c = chunk[i];
                if (c == '\n') {
                    lineBuf_[lineLen_] = '\0';
                    // Only parse lines that look like JSON (start with '{')
                    if (lineLen_ > 0 && lineBuf_[0] == '{') {
                        parseInput(lineBuf_, state);
                    }
                    lineLen_ = 0;
                } else if (c != '\r' && lineLen_ < (int)sizeof(lineBuf_) - 1) {
                    lineBuf_[lineLen_++] = c;
                }
            }
        } else {
            // No new data — hold last known state
            state = lastState_;
        }

        return true;
    }

    // Reset tilt baseline; next CALIBRATION_COUNT readings will re-establish rest position
    void recalibrateTilt() {
        baselineAccelX_ = 0.0f;
        baselineAccelY_ = 0.0f;
        calibrationSumX_ = 0.0f;
        calibrationSumY_ = 0.0f;
        calibrationSamples_ = 0;
        tiltCalibrated_ = false;
        std::cout << "[SERIAL] Tilt baseline reset, recalibrating..." << std::endl;
    }

private:
    const char* device_;
    int baud_;
    int fd_;
    char lineBuf_[512];
    int lineLen_ = 0;
    InputState lastState_ = {0, 0, 0, 0, false, false, false, false};
    bool lastButton1_ = false;
    bool lastButton2_ = false;
    bool lastButton3_ = false;
    bool lastButton4_ = false;

    // Tilt calibration (mirrors udp_input.hpp)
    static constexpr float TILT_DEADZONE = 0.25f;
    static constexpr float TILT_MAX = 2.5f;
    float baselineAccelX_ = 0.0f;
    float baselineAccelY_ = 0.0f;
    int calibrationSamples_ = 0;
    float calibrationSumX_ = 0.0f;
    float calibrationSumY_ = 0.0f;
    bool tiltCalibrated_ = false;
    static constexpr int CALIBRATION_COUNT = 10;
    int logCounter_ = 0;

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

    int tiltToJoystick(float accel) {
        if (fabs(accel) < TILT_DEADZONE) return 0;
        float sign = (accel > 0) ? 1.0f : -1.0f;
        float adjusted = fabs(accel) - TILT_DEADZONE;
        float range = TILT_MAX - TILT_DEADZONE;
        int value = (int)(sign * (adjusted / range) * 100.0f);
        if (value > 100) value = 100;
        if (value < -100) value = -100;
        return value;
    }

    void parseInput(const char* buffer, InputState& state) {
        float accelX = parseJsonFloat(buffer, "accelX");
        float accelY = parseJsonFloat(buffer, "accelY");
        int prox     = parseJsonInt(buffer, "prox");
        int b1       = parseJsonInt(buffer, "btn1");
        int b2       = parseJsonInt(buffer, "btn2");
        int b3       = parseJsonInt(buffer, "btn3");
        int b4       = parseJsonInt(buffer, "btn4");

        // Calibrate tilt baseline from first readings
        if (!tiltCalibrated_) {
            calibrationSumX_ += accelX;
            calibrationSumY_ += accelY;
            calibrationSamples_++;
            if (calibrationSamples_ >= CALIBRATION_COUNT) {
                baselineAccelX_ = calibrationSumX_ / CALIBRATION_COUNT;
                baselineAccelY_ = calibrationSumY_ / CALIBRATION_COUNT;
                tiltCalibrated_ = true;
                std::cout << "[SERIAL] Tilt calibrated - baseline X: " << baselineAccelX_
                          << ", Y: " << baselineAccelY_ << std::endl;
            }
        }

        float relativeX = accelX - baselineAccelX_;
        float relativeY = accelY - baselineAccelY_;

        state.joystick_x = tiltToJoystick(-relativeX);  // Inverted
        state.joystick_y = tiltToJoystick(relativeY);

        int proxRatio = (prox > 6553) ? 100 : (prox * 100 / 6553);
        state.potentiometer  = proxRatio;
        state.potentiometer2 = proxRatio;

        // Direct button mapping: btn1=dash, btn2=fire, btn3=shield, btn4=weapon toggle
        state.button1 = (b1 != 0);
        state.button2 = (b2 != 0);
        state.button3 = (b3 != 0);
        state.button4 = (b4 != 0);

        // Log tilt pipeline every 25 packets (~500ms at 50Hz)
        if (logCounter_++ % 25 == 0) {
            std::cout << "[TILT] raw(" << accelX << "," << accelY << ")"
                      << " rel(" << relativeX << "," << relativeY << ")"
                      << " joy(" << state.joystick_x << "," << state.joystick_y << ")"
                      << std::endl;
        }

        // Log button events with action names
        if (state.button1 && !lastButton1_) std::cout << "[BTN] 1 -> DASH" << std::endl;
        if (state.button2 && !lastButton2_) std::cout << "[BTN] 2 -> FIRE" << std::endl;
        if (state.button3 && !lastButton3_) std::cout << "[BTN] 3 -> SHIELD" << std::endl;
        if (state.button4 && !lastButton4_) std::cout << "[BTN] 4 -> WEAPON TOGGLE" << std::endl;
        if (!state.button1 && lastButton1_) std::cout << "[BTN] 1 released" << std::endl;
        if (!state.button2 && lastButton2_) std::cout << "[BTN] 2 released" << std::endl;
        if (!state.button3 && lastButton3_) std::cout << "[BTN] 3 released" << std::endl;
        if (!state.button4 && lastButton4_) std::cout << "[BTN] 4 released" << std::endl;
        lastButton1_ = state.button1;
        lastButton2_ = state.button2;
        lastButton3_ = state.button3;
        lastButton4_ = state.button4;

        lastState_ = state;
    }
};

#endif
