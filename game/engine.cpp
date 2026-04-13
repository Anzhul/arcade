
#include <iostream>
#include <cstring>
#include <csignal>
#include <time.h>
#include "led-matrix.h"
#include "graphics.h"
#include "game.hpp"
#include "input.hpp"
#include "udp_input.hpp"
#include "serial_input.hpp"
#include "gamepad_input.hpp"
#include <unistd.h>

using namespace rgb_matrix;

static volatile bool interrupt_received = false;

static void InterruptHandler(int signo) {
    interrupt_received = true;
}

void run_game(InputProvider& input)
{
  // Options for the LED matrix
  RGBMatrix::Options matrix_options;
  matrix_options.rows = 64;
  matrix_options.cols = 64;
  matrix_options.chain_length = 1;
  matrix_options.parallel = 1;
  matrix_options.hardware_mapping = "adafruit-hat";

  RuntimeOptions runtime_options;
  runtime_options.gpio_slowdown = 4;

  // Create the matrix
  RGBMatrix *matrix = CreateMatrixFromOptions(matrix_options, runtime_options);
  if (matrix == nullptr)
  {
    std::cerr << "Could not create matrix. Exiting." << std::endl;
    return;
  }

  // Fill the entire matrix with the color
  for (int row = 0; row < 64; ++row)
  {
    for (int col = 0; col < 64; ++col)
    {
      matrix->SetPixel(col, row, 0, 150, 0);
    }
  }

  Game game = Game();
  game.setup();

  for (int row = 0; row < 64; ++row){
    for (int col = 0; col < 64; ++col){
      matrix->SetPixel(col, row, 18, 10, 14);
    }
  }

  int clock = 0;
  InputState state = {0, 0, 0, false, false, false};  // Initialize to center

  const long FRAME_TIME_NS = 33333333L;  // 33ms per frame = 30 FPS
  struct timespec frameStart, frameEnd;

  while (!interrupt_received && input.read(state)){
    clock_gettime(CLOCK_MONOTONIC, &frameStart);

    game.update(state, matrix, clock);
    ++clock;

    // Sleep for the remainder of the frame
    clock_gettime(CLOCK_MONOTONIC, &frameEnd);
    long elapsed = (frameEnd.tv_sec - frameStart.tv_sec) * 1000000000L
                 + (frameEnd.tv_nsec - frameStart.tv_nsec);
    long remaining = FRAME_TIME_NS - elapsed;
    if (remaining > 0) {
        struct timespec sleepTime = {0, remaining};
        nanosleep(&sleepTime, nullptr);
    }
  }

  matrix->Clear();
  delete matrix;
}

void printUsage(const char* progName) {
    std::cout << "Usage: " << progName << " [options]" << std::endl;
    std::cout << "  --serial [device]    Use USB serial tilt controller (default: /dev/ttyACM0)" << std::endl;
    std::cout << "  --gamepad [device]   Use USB gamepad (default: /dev/input/js0)" << std::endl;
    std::cout << "  --udp [port]         Use UDP input (default: port 8888)" << std::endl;
    std::cout << "  (no args)            Default to serial input on /dev/ttyACM0" << std::endl;
}

int main(int argc, char* argv[]) {
    signal(SIGINT, InterruptHandler);
    signal(SIGTERM, InterruptHandler);

    if (argc > 1 && strcmp(argv[1], "--gamepad") == 0) {
        const char* device = (argc > 2) ? argv[2] : nullptr;
        GamepadInputProvider input(device);
        if (!input.init()) return 1;
        std::cout << "Starting game with USB gamepad..." << std::endl;
        run_game(input);
    } else if (argc > 1 && strcmp(argv[1], "--udp") == 0) {
        int port = (argc > 2) ? atoi(argv[2]) : 8888;
        UdpInputProvider input(port);
        if (!input.init()) return 1;
        std::cout << "Starting game with UDP input on port " << port << "..." << std::endl;
        run_game(input);
    } else if (argc > 1 && strcmp(argv[1], "--help") == 0) {
        printUsage(argv[0]);
        return 0;
    } else {
        // Default: USB serial tilt controller
        const char* device = (argc > 2 && strcmp(argv[1], "--serial") == 0) ? argv[2]
                           : (argc > 1  && strcmp(argv[1], "--serial") != 0) ? argv[1]
                           : "/dev/ttyACM0";
        SerialInputProvider input(device);
        if (!input.init()) return 1;
        std::cout << "Starting game with serial tilt controller on " << device << "..." << std::endl;
        run_game(input);
    }

    return 0;
}