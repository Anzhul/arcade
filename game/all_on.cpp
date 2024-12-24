#include "led-matrix.h"
#include "graphics.h"
#include <iostream>
#include <csignal>

using namespace rgb_matrix;

volatile bool interrupt_received = false;

// Function to handle Ctrl+C
void InterruptHandler(int signo) {
    interrupt_received = true;
}

int main() {
    // Set up signal handler for Ctrl+C
    signal(SIGINT, InterruptHandler);

    // Options for the LED matrix
    RGBMatrix::Options matrix_options;
    matrix_options.rows = 64;
    matrix_options.cols = 64;
    matrix_options.chain_length = 1;
    matrix_options.parallel = 1;
    matrix_options.brightness = 75;
    matrix_options.hardware_mapping = "adafruit-hat";

    RuntimeOptions runtime_options;
    runtime_options.gpio_slowdown = 4;

    // Create the matrix
    RGBMatrix *matrix = CreateMatrixFromOptions(matrix_options, runtime_options);
    if (matrix == nullptr) {
        std::cerr << "Could not create matrix. Exiting." << std::endl;
        return 1;
    }

    // Set the color (red in this example)
    Color color(15, 5, 70);

    // Fill the entire matrix with the color
    for (int row = 0; row < 64; ++row) {
        for (int col = 0; col < 32; ++col) {
            matrix->SetPixel(col, row, color.r, color.g, color.b);
        }
    }

    // Keep the display on until interrupted
    while (!interrupt_received) {
        usleep(100000);
    }

    // Clear the display and exit
    matrix->Clear();
    delete matrix;
    return 0;
}