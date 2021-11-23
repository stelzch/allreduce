#ifndef UTIL_HPP_
#define UTIL_HPP_

#include <chrono>
#include <vector>
#include <iterator>
#include <functional>

using std::vector;

namespace Util {
    void startTimer();
    double endTimer();
    double average(const vector<double> &v);
    double stddev(const vector<double> &v);
    void attach_debugger(bool condition);

    /** Zip through two iterators and call a function */
    template<class Iter1, class Iter2, typename F>
        void zip(
            Iter1 a, Iter1 a_end,
            Iter2 b, Iter2 b_end,
            F f) {
        while(a != a_end && b != b_end) {
            f(*a, *b);
            a++; b++;
        }

    }
}

#endif
