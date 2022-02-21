#ifndef SUMMATION_STRATEGY_HPP_
#define SUMMATION_STRATEGY_HPP_

#include <cstdint>
#include <vector>
#include <utility>
#include <mpi.h>

using std::vector;

class SummationStrategy {
public:
    /**
     * Create a new summation strategy
     * @param rank The rank of the currently running executable
     * @param n_summands A vector whose length is equal to the cluster size. Each rank is assigned the number of summands
     *                   that belongs to it.
     */
    SummationStrategy(uint64_t rank, vector<int> &n_summands, MPI_Comm comm = MPI_COMM_WORLD);

    /**
     * This will distribute the values that are to be added across the cluster. The vector is only expected to be valid
     * on the root rank
     * @param values The data that is to be summed up.
     */
    virtual void distribute(vector<double> &values);

    const vector<double>& getSummands();
    void setSummands(const vector<double>& summands);


    virtual double accumulate() = 0;

    virtual ~SummationStrategy();

    virtual const void printStats(void) const {
    }

protected:
    const vector<int> n_summands;
    const int rank, clusterSize;
    const uint64_t globalSize;
    vector<int> startIndex;
    vector<double> summands;
    const int ROOT_RANK = 0;
    const MPI_Comm comm;
};

#endif
