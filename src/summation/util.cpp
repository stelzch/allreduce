#include <chrono>
#include <vector>
#include <numeric>
#include <cmath>
#include <fstream>
#include <iostream>
#include <unistd.h>
#include "util.hpp"
#include <strings.h>

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

const bool Util::is_power2(const unsigned long int x) {
    /* If x is a power of 2, there is exactly one bit set. We determine the
     * index of the lowest bit set with ffsl. We then produce a number where
     * only that lowest bit is set using the shift operator. If that number
     * corresponds to our input, it is a power of 2.
     *
     * The case where x == 0 must be considered carefully. Since ffsl(0) will
     * return 0, our lowest_bit_set will evaluate to -1 and therefore underflow
     * to numeric_limits<unsigned long>::max.  only_lowest_bit_set will then be
     * the integer where only the most-significant bit has been set (verified
     * experimentally) and thus will not equal 0, so the function  will
     * correctly evaluate to false.
     */
    const unsigned int lowest_bit_set = ffsl(x) - 1;
    const unsigned long int only_lowest_bit_set = (1 << lowest_bit_set);
    return (x == only_lowest_bit_set);
}
