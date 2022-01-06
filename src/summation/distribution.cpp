#include <distribution.hpp>
#include <strategies/binary_tree.hpp>
#include <vector>
#include <cstdint>
#include <iostream>
#include <cassert>
#include <algorithm>
#include <cmath>
#include <string>
#include <numeric>
#include <sstream>

using std::vector;
using std::cout;
using std::endl;
using std::string;
using std::stringstream;

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
        
        uint64_t n = (i >=  ranks - remainder) ? (perRank + 1) : perRank;
        d.nSummands[i] = n;
        index += n;
    }

    return d;
}

const bool Distribution::varianceWithinBounds(const uint64_t fairIndex, const uint64_t proposedIndex, const uint64_t fairShare,
        const float variance) {
    float deviationFromFairIndex = abs(fairIndex - proposedIndex) / static_cast<float>(fairShare);
    return deviationFromFairIndex <= variance;
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
                && Distribution::varianceWithinBounds(fairShare * i, proposedIndex, fairShare, variance)) {
            index = proposedIndex;
            if (i % 2 == 0) {
                proposedIndex = BinaryTreeSummation::parent(index);
            } else {
                proposedIndex = Distribution::roundUp(proposedIndex);
            }
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

const Distribution Distribution::optimal(const uint64_t n, const uint64_t ranks) {
    Distribution candidate(1,1);
    double score = INFINITY;
    double candidateVariance = 1.0;
    bool hasBeenDescending = false;


    for (double testedVariance = 0.0; testedVariance < 1.0; testedVariance += 0.0001) {
        auto generated = Distribution::lsb_cleared(n, ranks, testedVariance);

        if (generated.score() < score) {
            candidate = generated;
            score = generated.score();
            candidateVariance = testedVariance;
            hasBeenDescending = true;
        }

        if (generated.score() > score && hasBeenDescending) {
            // Reached saddle point, do not optimize further
            break;
        }
    }

    cout << "Best optimization with variance " << candidateVariance << endl;

    return candidate;
}

const Distribution Distribution::from_string(const string description) {
    string spec(description);
    std::replace(spec.begin(), spec.end(), ',', ' ');

    vector<uint64_t> nSummands;
    stringstream ss(spec);
    uint64_t mn;

    while (ss >> mn) {
        nSummands.push_back(mn);
    }

    const uint64_t n = std::accumulate(nSummands.begin(), nSummands.end(), 0L);
    const uint64_t ranks = nSummands.size();

    Distribution d(n, ranks);

    uint64_t startIndex = 0;
    for (uint64_t i = 0; i < ranks; i++) {
        d.startIndices[i] = startIndex;
        d.nSummands[i] = nSummands[i];
        startIndex += nSummands[i];
    }

    return d;
}

static const Distribution upDown(const uint64_t n, const uint64_t ranks) {
    
}

const uint64_t Distribution::rankIntersectionCount() const {
    if (rankIntersectionCountValid) {
        return _rankIntersectionCount;
    }

    uint64_t totalRankIntersections = 0;
    for (uint64_t i = 1; i < startIndices.size(); i++) {
        const uint64_t startIndex = startIndices[i];

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
    const double t_send = 110e-9;
    const double t_doubleadd = 2.44e-9;

    return t_send * rankIntersectionCount() + // communication
        *std::max_element(nSummands.begin(), nSummands.end()) * t_doubleadd;  // calculation
}

const void Distribution::printScore() const {
    const uint64_t min = *std::min_element(nSummands.begin(), nSummands.end());
    const uint64_t max = *std::max_element(nSummands.begin(), nSummands.end());

    const uint64_t diff = max - min;
    printf("%f, (%li messages), %li maximal message difference\n", score(), rankIntersectionCount(), diff);

}

const void Distribution::printDistribution() const {
    cout << "[" << nSummands[0];
    for (auto it = nSummands.begin() + 1; it != nSummands.end(); it++) {
        cout << ", " << *it;
    }
    cout << "]" << endl;
}

const uint64_t Distribution::roundUp(const uint64_t number) {
    const int delta = ffsl(number) - 1;
    // Search loop for first zero with an index greater than delta
    for (int i = delta; i < sizeof(number) * 8; i++) {

        // If we have found a bit, set it and reset all less significant bits
        if ((number & (1UL << i)) == 0) {
           const uint64_t mask = ~((1 << i) - 1); // Clear all bits lower than i
           const uint64_t roundedNumber = (number & mask) | (1 << i); // Set bit i

           return roundedNumber;
        }
    }

    // If the search is unsuccessful, return the input number.
    return number;
}
