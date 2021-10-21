#include <chrono>
#include "util.hpp"

std::chrono::time_point<std::chrono::high_resolution_clock> last;

void Util::startTimer() {
    last = std::chrono::high_resolution_clock::now();
}

double Util::endTimer() {
    auto now = std::chrono::high_resolution_clock::now();
    double microseconds = std::chrono::duration_cast<std::chrono::microseconds>(now - last).count();
    return microseconds;
}
