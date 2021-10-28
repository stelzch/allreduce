#include "summation_strategy.hpp"

#include <utility>
#include <algorithm>
#include <cassert>
#include <iostream>
#include <mpi.h>
#include <numeric>
#include <limits>

SummationStrategy::SummationStrategy(uint64_t rank, vector<int> &n_summands)
    : rank(rank),
      n_summands(n_summands),
      globalSize(std::accumulate(n_summands.begin(), n_summands.end(), 0L)),
      clusterSize(n_summands.size()) {

    summands.resize(n_summands[rank]);
    startIndex.reserve(n_summands.size());

    /* Fill startIndex vector with the index of the first element */
    uint64_t index = 0;
    for (uint64_t n : n_summands) {
        startIndex.push_back(index);
        index += n;
    }
}

SummationStrategy::~SummationStrategy() {

}

void SummationStrategy::distribute(vector<double> &values) {
    MPI_Scatterv(&values[0], &n_summands[0],&startIndex[0], MPI_DOUBLE,
                 &summands[0], n_summands[rank], MPI_DOUBLE,
                 ROOT_RANK, MPI_COMM_WORLD);
}