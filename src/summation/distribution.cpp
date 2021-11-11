#include <distribution.hpp>
#include <strategies/binary_tree.hpp>
#include <vector>
#include <cstdint>
#include <iostream>
#include <cassert>
#include <cmath>

using std::vector;
using std::cout;
using std::endl;

Distribution::Distribution(uint64_t n, uint64_t ranks)
        : n(n), ranks(ranks), nSummands(ranks), startIndices(ranks), _rankIntersectionCount(-1) {
}

const Distribution Distribution::even(uint64_t n, uint64_t ranks) {
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

const Distribution Distribution::even_remainder_on_last(uint64_t n, uint64_t ranks) {
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

const bool Distribution::varianceWithinBounds(const uint64_t actualLength, const uint64_t intendedLength,
        const float variance) {
    float proportion = actualLength / (float)  intendedLength;
    return variance <= proportion;
}

const Distribution Distribution::lsb_cleared(const uint64_t n, const uint64_t ranks, const float variance) {
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
            proposedIndex = BinaryTreeSummation::parent(index);
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

const uint64_t Distribution::rankIntersectionCount() const {
    if (rankIntersectionCountValid) {
        return _rankIntersectionCount;
    }

    uint64_t totalRankIntersections = 0;
    for (int i = 1; i < startIndices.size(); i++) {
        const uint64_t startIndex = startIndices[i];
        const uint64_t previousStartIndex = startIndices[i - 1];

        uint64_t rankIntersections = 0;
        for (uint64_t numberIndex = startIndex; numberIndex < startIndex + nSummands[i]; numberIndex++) {
            if (BinaryTreeSummation::parent(numberIndex) < startIndex)
                rankIntersections++;
        }

        totalRankIntersections += rankIntersections;
    }

    _rankIntersectionCount = totalRankIntersections;
    rankIntersectionCountValid = true;
    return totalRankIntersections;
}

const double Distribution::score() const {
    const double t_send = 281e-9;
    const double t_doubleadd = 4.15e-9;

    return t_send * rankIntersectionCount() + // communication
        *std::max_element(nSummands.begin(), nSummands.end()) * t_doubleadd;  // calculation
}

const void Distribution::printScore() const {
    printf("%f, (%li messages)\n", score(), rankIntersectionCount());
}

const void Distribution::printDistribution() const {
    cout << "[" << nSummands[0];
    for (auto it = nSummands.begin() + 1; it != nSummands.end(); it++) {
        cout << ", " << *it;
    }
    cout << "]" << endl;
}
