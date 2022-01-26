#include "summation_strategy.hpp"

#include <utility>
#include <algorithm>
#include <cassert>
#include <iostream>
#include <mpi.h>
#include <numeric>
#include <limits>

SummationStrategy::SummationStrategy(uint64_t rank, vector<int> &n_summands, MPI_Comm comm)
    : n_summands(n_summands),
      rank(rank),
      clusterSize(n_summands.size()),
      globalSize(std::accumulate(n_summands.begin(), n_summands.end(), 0L)),
      comm(comm) {

    assert(globalSize > 0);

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
    if (n_summands.size() == 1) {
        // MPI-less variant for testing
        assert(values.size() >= summands.size());
        for (uint64_t i = 0; i < summands.size(); i++) {
            summands[i] = values[i];
        }
    } else {
	int comm_size;
	MPI_Comm_size(MPI_COMM_WORLD, &comm_size);

	// Our comm size might differ from the number of summands (if n < m for example)
	vector<int> sendCounts = n_summands;
	vector<int> displacements = startIndex;

        MPI_Scatterv(&values[0], &sendCounts[0], &displacements[0], MPI_DOUBLE,
                    &summands[0], n_summands[rank], MPI_DOUBLE,
                    ROOT_RANK, comm);
    }
}

const vector<double>& SummationStrategy::getSummands() {
    return summands;
}
