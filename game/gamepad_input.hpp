#ifndef GAMEPAD_INPUT_HPP
#define GAMEPAD_INPUT_HPP

#include "input.hpp"
#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <dirent.h>
#include <sys/ioctl.h>

class GamepadInputProvider : public InputProvider {
public:
    GamepadInputProvider(const char* device = nullptr) : fd_(-1), running_(false) {
        if (device) {
            strncpy(devicePath_, device, sizeof(devicePath_) - 1);
            devicePath_[sizeof(devicePath_) - 1] = '\0';
        } else {
            devicePath_[0] = '\0';  // Auto-detect
        }
    }

    ~GamepadInputProvider() {
        if (fd_ >= 0) {
            close(fd_);
        }
    }

    bool init() {
        // Auto-detect gamepad if no path specified
        if (devicePath_[0] == '\0') {
            const char* found = findGamepad();
            if (found) {
                strncpy(devicePath_, found, sizeof(devicePath_) - 1);
            } else {
                std::cerr << "Error: No USB gamepad found." << std::endl;
                return false;
            }
        }

        fd_ = open(devicePath_, O_RDONLY | O_NONBLOCK);
        if (fd_ < 0) {
            std::cerr << "Error: Could not open gamepad at " << devicePath_ << std::endl;
            return false;
        }

        char name[128] = "Unknown";
        ioctl(fd_, EVIOCGNAME(sizeof(name)), name);
        std::cout << "Gamepad connected: " << name << " (" << devicePath_ << ")" << std::endl;

        running_ = true;
        return true;
    }

    bool read(InputState& state) override {
        if (!running_ || fd_ < 0) {
            return false;
        }

        struct input_event ev;

        // Read all pending evdev events (non-blocking)
        while (::read(fd_, &ev, sizeof(ev)) == sizeof(ev)) {
            if (ev.type == EV_ABS) {
                handleAxis(ev.code, ev.value);
            } else if (ev.type == EV_KEY) {
                handleButton(ev.code, ev.value);
            }
        }

        // D-pad: evdev range 0-255, center=127
        // Map to -100..100 and ramp smoothly (digital D-pad)
        int targetX = scaleAxis(rawAxes_[0]);
        int targetY = scaleAxis(rawAxes_[1]);
        smoothX_ = ramp(smoothX_, targetX, RAMP_SPEED);
        smoothY_ = ramp(smoothY_, targetY, RAMP_SPEED);
        state.joystick_x = smoothX_;
        state.joystick_y = smoothY_;

        // 292 = decrease shield/fire ratio, 293 = increase
        if (rawButtons_[292] && !prevButtons_[292]) {
            potValue_ -= POT_STEP;
            if (potValue_ < 0) potValue_ = 0;
            std::cout << "[GAMEPAD] Shield/Fire ratio: " << potValue_ << "%" << std::endl;
        }
        if (rawButtons_[293] && !prevButtons_[293]) {
            potValue_ += POT_STEP;
            if (potValue_ > 100) potValue_ = 100;
            std::cout << "[GAMEPAD] Shield/Fire ratio: " << potValue_ << "%" << std::endl;
        }
        state.potentiometer = potValue_;

        // 291 = cycle weapon mode
        if (rawButtons_[291] && !prevButtons_[291]) {
            weaponMode_ = (weaponMode_ + 1) % 4;
            std::cout << "[GAMEPAD] Weapon mode: " << weaponMode_ << std::endl;
        }
        state.potentiometer2 = weaponMode_ * 25;

        // Button mapping:
        // 289 = Fire (button2)
        // 288 = Shield bubble (button3)
        // 290 = Dash (button1)
        // 291 = Weapon toggle (button4)
        state.button1 = rawButtons_[290];  // Dash
        state.button2 = rawButtons_[289];  // Fire
        state.button3 = rawButtons_[288];  // Shield bubble
        state.button4 = rawButtons_[291];  // Weapon toggle

        // Save previous button state for edge detection
        memcpy(prevButtons_, rawButtons_, sizeof(prevButtons_));

        return running_;
    }

    void stop() {
        running_ = false;
    }

private:
    char devicePath_[64];
    int fd_;
    bool running_;

    // Raw evdev state
    int rawAxes_[8] = {127, 127, 0, 0, 0, 0, 0, 0};  // Center at 127
    bool rawButtons_[304] = {false};   // Evdev button codes start at 288
    bool prevButtons_[304] = {false};

    // Smoothed joystick state
    int smoothX_ = 0;
    int smoothY_ = 0;

    // Derived state
    int potValue_ = 50;
    int weaponMode_ = 0;

    static constexpr int POT_STEP = 10;
    static constexpr int AXIS_DEADZONE = 20;   // Deadzone around center (127 +/- 20)
    static constexpr int RAMP_SPEED = 18;

    int scaleAxis(int raw) {
        // Evdev range: 0-255, center=127
        int centered = raw - 127;
        if (centered > -AXIS_DEADZONE && centered < AXIS_DEADZONE) {
            return 0;
        }
        // Map -127..127 to -100..100
        int val = (centered * 100) / 127;
        if (val > 100) val = 100;
        if (val < -100) val = -100;
        return val;
    }

    int ramp(int current, int target, int speed) {
        if (current < target) {
            current += speed;
            if (current > target) current = target;
        } else if (current > target) {
            current -= speed;
            if (current < target) current = target;
        }
        return current;
    }

    void handleAxis(int code, int value) {
        // ABS_X=0, ABS_Y=1
        if (code < 8) {
            rawAxes_[code] = value;
        }
    }

    void handleButton(int code, int value) {
        if (code >= 288 && code < 304) {
            rawButtons_[code] = (value != 0);
        }
    }

    // Find the first USB gamepad evdev device
    const char* findGamepad() {
        static char path[64];
        DIR* dir = opendir("/dev/input");
        if (!dir) return nullptr;

        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (strncmp(entry->d_name, "event", 5) != 0) continue;
            snprintf(path, sizeof(path), "/dev/input/%s", entry->d_name);

            int fd = open(path, O_RDONLY);
            if (fd < 0) continue;

            char name[128] = "";
            ioctl(fd, EVIOCGNAME(sizeof(name)), name);
            close(fd);

            // Look for "Gamepad" or "gamepad" in the name
            if (strstr(name, "amepad") || strstr(name, "oystick")) {
                closedir(dir);
                return path;
            }
        }
        closedir(dir);
        return nullptr;
    }
};

#endif
