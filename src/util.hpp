#pragma once

#include <chrono>
#include <vector>

using std::vector;

namespace Util {
    void startTimer();
    double endTimer();
    const double average(const vector<double> &v);
    double stddev(const vector<double> &v);
    void attach_debugger(bool condition);
}
