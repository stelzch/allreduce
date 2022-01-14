#ifndef CRITICAL_PATH_HPP_
#define CRITICAL_PATH_HPP_

#include <cstdint>
#include <map>

using std::map;

struct Info {
    uint64_t n;
    map<uint64_t, int> startIndices;
    double t_add;
    double t_send;
    bool print_ri;

    Info(const uint64_t n, const int m);
    const double tree(const uint64_t x, const uint64_t y) const;
    const uint64_t rankFromIndexMap(const uint64_t index) const;

    const double time(void) const;
};
#endif
