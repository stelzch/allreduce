#include <chrono>
#include <vector>
#include <numeric>
#include <cmath>
#include <fstream>
#include <iostream>
#include <unistd.h>
#include "util.hpp"

using std::vector;
using std::cout;
using std::endl;
using std::ofstream;

std::chrono::time_point<std::chrono::high_resolution_clock> last;

void Util::startTimer() {
    last = std::chrono::high_resolution_clock::now();
}

double Util::endTimer() {
    auto now = std::chrono::high_resolution_clock::now();
    double microseconds = std::chrono::duration_cast<std::chrono::microseconds>(now - last).count();
    return microseconds;
}

double Util::average(const vector<double> &v) {
    return std::accumulate(v.begin(), v.end(), 0.0) / (double) v.size();
}

double Util::stddev(const vector<double> &v) {
    const double avg = Util::average(v);
    double accumulator = 0.0;

    for (const double x : v)
        accumulator += pow(x - avg, 2.0);

    return sqrt(accumulator / (double) v.size());
}

void __attribute__((optimize("O0"))) Util::attach_debugger(bool condition) {
    if (!condition) return;
    bool attached = false;

    // also write PID to a file
    ofstream os("/tmp/mpi_debug.pid");
    os << getpid() << endl;
    os.close();

    cout << "Waiting for debugger to be attached, PID: "
        << getpid() << endl;
    while (!attached) sleep(1);
}
