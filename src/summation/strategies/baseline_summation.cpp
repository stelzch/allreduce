//
// Created by christoph on 28.10.21.
//

#include "baseline_summation.hpp"
#include <mpi.h>
#include <numeric>

double BaselineSummation::accumulate() {
    std::vector<double> allSummands;
    allSummands.resize(globalSize);

    MPI_Gatherv(&summands[0], n_summands[rank], MPI_DOUBLE,
                &allSummands[0],
                reinterpret_cast<const int *>(&n_summands[0]),
                reinterpret_cast<const int*>(&startIndex[0]),
                MPI_DOUBLE, ROOT_RANK, comm
    );

    double globalSum;
    if (rank == ROOT_RANK) {
        globalSum = std::accumulate(allSummands.begin(), allSummands.end(), 0.0);
    }
    MPI_Bcast(&globalSum, 1, MPI_DOUBLE,
              ROOT_RANK, comm);

    return globalSum;
}
