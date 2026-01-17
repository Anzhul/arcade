
#include <iostream>
#include <csignal>
#include "led-matrix.h"
#include "graphics.h"
#include "game.hpp"
#include "input.hpp"
#include "udp_input.hpp"
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

  while (!interrupt_received && input.read(state)){
    game.update(state, matrix, clock);

    usleep(10000);  // 10ms per frame (~100 fps)
    ++clock;
  }

  matrix->Clear();
  delete matrix;
}

int main(int argc, char* argv[]) {
    signal(SIGINT, InterruptHandler);
    signal(SIGTERM, InterruptHandler);

    int port = 8888;
    if (argc > 1) {
        port = atoi(argv[1]);
    }

    UdpInputProvider input(port);
    if (!input.init()) {
        return 1;
    }

    std::cout << "Starting game, waiting for controller input..." << std::endl;
    run_game(input);

    return 0;
}