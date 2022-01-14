#include "critical_path.hpp"
#include <algorithm>
#include <map>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

using namespace std;

/* Initialize start indices map */
const map<uint64_t, int> calcStartIndicesMap(const uint64_t n, const int m) {
    map<uint64_t, int> result;

    uint64_t perRank = floor(n / m);
    uint64_t remainder = n % m;

    int rank = 0;
    int startIndex = 0;
    for (int i = 0; i < m; i++) {
        const uint64_t n = (i >=  m - remainder) ? (perRank + 1) : perRank;
        result[startIndex] = rank++;
        startIndex += n;
    }

    // guardian element
    result[startIndex] = rank;

    return result;
}





Info::Info(const uint64_t n, const int m) : n(n) {
    startIndices = calcStartIndicesMap(n, m);
}

const double Info::tree(const uint64_t x, const uint64_t y) const {
    if (y == 0) {
        // Leaf
        //printf("Leaf (%li, %li)\n", x, y);
        return 0.0;
    } else {
        const uint64_t x_a = x;
        const uint64_t y_a = y - 1;

        const uint64_t x_b = x + (1UL << (y - 1));
        const uint64_t y_b = y - 1;

        if (x_b >= n) {
            // Inner Node

            // Simply pass the completion time point through
            const double r = tree(x_a, y_a);
            //printf("InnerNode (%li, %li)\n = %f", x, y, r);

            return r;
        } else if (rankFromIndexMap(x_a) != rankFromIndexMap(x_b)) {
            // Rank Intersection
            const double t1 = tree(x_a, y_a);
            const double t2 = tree(x_b, y_b);

            const double r = t_add + t_send + max(t1, t2);

            //printf("RankIntersection (%li, %li) = %f\n", x, y, r);
            return r;
        } else {
            // Addition Node
            const double t1 = tree(x_a, y_a);
            const double t2 = tree(x_b, y_b);

            const double r = t_add + t1 + t2;
            //printf("AdditionNode (%li, %li) = %f\n", x, y, r);
            return r;
        }

    }
}

const uint64_t Info::rankFromIndexMap(const uint64_t index) const {
    auto it = startIndices.upper_bound(index);

    //assert(it != startIndices.end());

    const int nextRank = it->second;
    return nextRank - 1;
}

const double Info::time(void) const {
    const uint64_t maxY = ceil(log2(n));
    return tree(0, maxY);
}

#ifdef CRITICAL_PATH_MAIN
int main(int argc, char **argv) {
    if (argc != 4 + 1) {
        cerr << "Usage: " << argv[0] << " <n> <m> <t_send> <t_add>" << endl;
        return -1;
    }

    const uint64_t n = atoi(argv[1]);
    const uint64_t m = atoi(argv[2]);
    const double t_send = atof(argv[3]);
    const double t_add = atof(argv[4]);


    Info i(n, m);
    i.t_add = t_add;
    i.t_send = t_send;

    printf("Σ = %f\n", i.time());

    return 0;
}
#endif
