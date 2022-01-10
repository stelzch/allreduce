#ifndef SUMMATION_STRATEGY_HPP_
#define SUMMATION_STRATEGY_HPP_

#include <cstdint>
#include <vector>
#include <utility>

using std::vector;

class SummationStrategy {
public:
    /**
     * Create a new summation strategy
     * @param rank The rank of the currently running executable
     * @param n_summands A vector whose length is equal to the cluster size. Each rank is assigned the number of summands
     *                   that belongs to it.
     */
    SummationStrategy(uint64_t rank, vector<int> &n_summands);

    /**
     * This will distribute the values that are to be added across the cluster. The vector is only expected to be valid
     * on the root rank
     * @param values The data that is to be summed up.
     */
    void distribute(vector<double> &values);


    virtual double accumulate() = 0;

    virtual ~SummationStrategy();

    virtual const std::pair<size_t, size_t> messageStat() const {
        return std::make_pair(0, 0);
    }

protected:
    const vector<int> n_summands;
    const int rank, clusterSize;
    const uint64_t globalSize;
    vector<int> startIndex;
    vector<double> summands;
    const int ROOT_RANK = 0;
};

#endif
