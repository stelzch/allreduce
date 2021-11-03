#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>
#include "util.hpp"

using namespace std;

const double t_send = 281e-9;
const double t_doubleadd = 4.15e-9;

const uint64_t parent(const uint64_t i) {
    return i & (i - 1);
}

struct Distribution {
    uint64_t n;         // number of summands
    uint64_t ranks;     // number of ranks
    vector<uint64_t> nSummands;
    vector<uint64_t> startIndices;

    Distribution(uint64_t n, uint64_t ranks)
        : n(n), ranks(ranks), nSummands(ranks), startIndices(ranks), _rankIntersectionCount(-1) {
        }

    /* Generate an even distribution where the numbers are spread evenly among the hosts
     * and the remainder is put on the first few hosts (1 number each)
     */
    static const Distribution even(uint64_t n, uint64_t ranks) {
        Distribution d(n, ranks);

        uint64_t perRank = floor(n / ranks);
        uint64_t remainder = n % ranks;

        uint64_t index = 0;
        for (uint64_t i = 0; i < ranks; i++) {
            d.startIndices[i] = index;
            
            uint64_t n = (i < remainder) ? (perRank + 1) : perRank;
            d.nSummands[i] = n;
            index += n;
        }

        return d;
    }

    /* Generate an even distribution where the numbers are spread evenly among the hosts
     * and the remainder is put on the last host
     */
    static const Distribution even_remainder_on_last(uint64_t n, uint64_t ranks) {
        Distribution d(n, ranks);

        uint64_t perRank = floor(n / ranks);
        uint64_t remainder = n % ranks;

        uint64_t index = 0;
        for (uint64_t i = 0; i < ranks; i++) {
            d.startIndices[i] = index;
            
            uint64_t n = (i ==  ranks - 1) ? (perRank + remainder) : perRank;
            d.nSummands[i] = n;
            index += n;
        }

        return d;
    }

    static const bool varianceWithinBounds(const uint64_t actualLength, const uint64_t intendedLength,
            const float variance) {
        float proportion = actualLength / (float)  intendedLength;
        return variance <= proportion;
    }

    static const Distribution lsb_cleared(const uint64_t n, const uint64_t ranks, const float variance) {
        assert(0 <= variance);

        Distribution d(n, ranks);

        uint64_t fairShare = floor(n / ranks);

        d.startIndices[0] = 0;
        for (uint64_t i = 1; i < ranks; i++) {
            const uint64_t lastIndex = d.startIndices[i - 1];

            // By default, we start with a fair share
            uint64_t proposedIndex = lastIndex + fairShare;
            uint64_t index = proposedIndex;

            // Then we iterate on the index, trying to optimize it. As long as we are not assigning
            // a negative amount of summands and the deviation is within the requested bounds,
            // we replace the proposed index with its parent index, producing one least-significant zero
            // per iteration.
            while(lastIndex < proposedIndex
                    && varianceWithinBounds(proposedIndex - lastIndex, fairShare, variance)) {
                index = proposedIndex;
                proposedIndex = parent(index);
            }

            // By now, proposedIndex violates one of the two conditions above and the best we were able to
            // achieve is stored in the variable index.
            d.startIndices[i] = index;

            // By this point, we also know how many numbers are assigned to the last rank.
            d.nSummands[i - 1] = index - lastIndex;
        }
        // All the remaining numbers will be put on the last rank
        d.nSummands[ranks - 1] = n - d.startIndices[ranks - 1];

        assert(d.nSummands.size() == ranks);
        assert(d.startIndices.size() == ranks);

        return d;
    }

    const uint64_t rankIntersectionCount() const {
        if (rankIntersectionCountValid) {
            return _rankIntersectionCount;
        }

        uint64_t totalRankIntersections = 0;
        for (int i = 1; i < startIndices.size(); i++) {
            const uint64_t startIndex = startIndices[i];
            const uint64_t previousStartIndex = startIndices[i - 1];

            uint64_t rankIntersections = 0;
            for (uint64_t numberIndex = startIndex; numberIndex < startIndex + nSummands[i]; numberIndex++) {
                if (parent(numberIndex) < startIndex)
                    rankIntersections++;
            }

            totalRankIntersections += rankIntersections;
        }

        _rankIntersectionCount = totalRankIntersections;
        rankIntersectionCountValid = true;
        return totalRankIntersections;
    }

    const double score() const {
        return t_send * rankIntersectionCount() + // communication
            *std::max_element(nSummands.begin(), nSummands.end()) * t_doubleadd;  // calculation
    }

    const void printScore() const {
        printf("%f, (%li messages)\n", score(), rankIntersectionCount());
    }

    const void printDistribution() const {
        cout << "[" << nSummands[0];
        for (auto it = nSummands.begin() + 1; it != nSummands.end(); it++) {
            cout << ", " << *it;
        }
        cout << "]" << endl;
    }

    private:
        mutable uint64_t _rankIntersectionCount;
        mutable bool rankIntersectionCountValid = false;

};



int main(int argc, char **argv) {
    if (argc != 4) {
        cout << "Usage: " << argv[0] << " <number of summands> <number of ranks> <variance>" << endl;
        return -1;
    }

    const uint64_t n = std::atoi(argv[1]);
    const uint64_t ranks = std::atoi(argv[2]);
    const float variance = std::atof(argv[3]);
    if (n == 0 || ranks == 0) {
        cerr << "Error: zero ranks or summands makes no sense." << endl;
        return -1;
    }

    auto even = Distribution::even(n, ranks);
    auto optimized = Distribution::lsb_cleared(n, ranks, variance);

    cout << "Even: "; even.printScore();
    even.printDistribution();
    cout << endl;

    cout << "Optimized: "; optimized.printScore();
    optimized.printDistribution();

}
