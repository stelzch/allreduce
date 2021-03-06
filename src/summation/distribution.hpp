//
// Created by christoph on 29.10.21.
//

#ifndef BINOMIALALLREDUCE_DATA_DISTRIBUTION_HPP
#define BINOMIALALLREDUCE_DATA_DISTRIBUTION_HPP

#include <string>
#include <vector>
#include <cstdint>

using std::vector;
using std::string;

struct Distribution {
    uint64_t n;         // number of summands
    uint64_t ranks;     // number of ranks
    vector<uint64_t> nSummands;
    vector<uint64_t> startIndices;

    Distribution(uint64_t n, uint64_t ranks);

    /* Generate an even distribution where the numbers are spread evenly among the hosts
     * and the remainder is put on the first few hosts (1 number each)
     */
    static const Distribution even(uint64_t n, uint64_t ranks);

    /* Generate an even distribution where the numbers are spread evenly among the hosts
     * and the remainder is put on the last host
     */
    static const Distribution even_remainder_on_last(uint64_t n, uint64_t ranks);

    static const bool varianceWithinBounds(const uint64_t fairIndex, const uint64_t proposedIndex, const uint64_t fairShare,
            const float variance);

    static const Distribution lsb_cleared(const uint64_t n, const uint64_t ranks, const float variance);

    static const Distribution optimal(const uint64_t n, const uint64_t ranks);

    static const Distribution from_string(const string description);

    const uint64_t rankIntersectionCount() const;

    const double score() const;

    const void printScore() const;

    const void printDistribution() const;

    static const uint64_t roundUp(const uint64_t index);

    private:
        mutable uint64_t _rankIntersectionCount;
        mutable bool rankIntersectionCountValid = false;


};

#endif //BINOMIALALLREDUCE_DATA_DISTRIBUTION_HPP
