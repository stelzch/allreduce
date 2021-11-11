//
// Created by christoph on 29.10.21.
//

#ifndef BINOMIALALLREDUCE_DATA_DISTRIBUTION_HPP
#define BINOMIALALLREDUCE_DATA_DISTRIBUTION_HPP

#include <vector>
#include <cstdint>

using std::vector;

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

    static const bool varianceWithinBounds(const uint64_t actualLength, const uint64_t intendedLength,
            const float variance);

    static const Distribution lsb_cleared(const uint64_t n, const uint64_t ranks, const float variance);

    const uint64_t rankIntersectionCount() const;

    const double score() const;

    const void printScore() const;

    const void printDistribution() const;

    private:
        mutable uint64_t _rankIntersectionCount;
        mutable bool rankIntersectionCountValid = false;

};

#endif //BINOMIALALLREDUCE_DATA_DISTRIBUTION_HPP
