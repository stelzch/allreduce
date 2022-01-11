
#include <algorithm>
#include <map>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <util.hpp>
#include <distribution.hpp>

using namespace std;

/* Initialize start indices map */
const map<uint64_t, int> calcStartIndicesMap(const Distribution &d) {
    map<uint64_t, int> result;

    int rank = 0;
    int startIndex = 0;
    for (const int &n : d.nSummands) {
        result[startIndex] = rank++;
        startIndex += n;
    }

    // guardian element
    result[startIndex] = rank;

    return result;
}

uint64_t rankFromIndexMap(const map<uint64_t, int> startIndices, const uint64_t index) {
    auto it = startIndices.upper_bound(index);

    //assert(it != startIndices.end());

    const int nextRank = it->second;
    return nextRank - 1;
}


void tree(const uint64_t n, const map<uint64_t, int> startIndices,
        const uint64_t x, const uint64_t y) {
    if (y == 0) {
        // Leaf
        printf("Leaf (%li, %li)\n", x, y);
    } else {
        const uint64_t x_a = x;
        const uint64_t y_a = y - 1;

        const uint64_t x_b = x + (1UL << (y - 1));
        const uint64_t y_b = y - 1;

        if (x_b >= n) {
            // Inner Node
            printf("InnerNode (%li, %li)\n", x, y);
            tree(n, startIndices, x_a, y_a);
        } else if (rankFromIndexMap(startIndices, x_a) != rankFromIndexMap(startIndices, x_b)) {
            // Rank Intersection
            printf("RankIntersection (%li, %li)\n", x, y);
            tree(n, startIndices, x_a, y_a); tree(n, startIndices, x_b, y_b);
        } else {
            // Addition Node
            printf("AdditionNode (%li, %li)\n", x, y);
            tree(n, startIndices, x_a, y_a);
            tree(n, startIndices, x_b, y_b);

        }

    }
}

int __attribute__((optimize("O0"))) main(int argc, char **argv) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <n> <m>" << endl;
        return -1;
    }

    const uint64_t n = atoi(argv[1]);
    const uint64_t m = atoi(argv[2]);

    const auto d = Distribution::even_remainder_on_last(n, m);

    const uint64_t maxY = ceil(log2(n));
    tree(n, calcStartIndicesMap(d), 0, maxY);

    return 0;
}
