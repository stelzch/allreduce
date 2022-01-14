//
// Created by christoph on 28.10.21.
//

#include "allreduce_summation.hpp"
#include <execution>
#include <numeric>
#include <mpi.h>

double AllreduceSummation::accumulate() {
    double localSum = std::accumulate(summands.begin(), summands.end(), 0.0);
    double globalSum = 0.0;
    MPI_Allreduce(&localSum, &globalSum, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

    return globalSum;
}
