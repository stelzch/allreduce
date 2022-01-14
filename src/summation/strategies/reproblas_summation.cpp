#include "reproblas_summation.hpp"
#include <mpi.h>

extern "C" {
#include <binned.h>
#include <binnedBLAS.h>
#include <reproBLAS.h>
#include <binnedMPI.h>
}


double ReproBLASSummation::accumulate() {
    /* Adopted from the MPI_sum_sine.c example, line 105 onwards */
    double_binned *isum = NULL;
    double_binned *local_isum = binned_dballoc(3);
    binned_dbsetzero(3, local_isum);

    if (rank == 0) {
        isum = binned_dballoc(3);
        binned_dbsetzero(3, isum);
    }

    // Local summation
    binnedBLAS_dbdsum(3, n_summands[rank], &summands[0], 1, local_isum);

    MPI_Reduce(local_isum, isum, 1, binnedMPI_DOUBLE_BINNED(3),
            binnedMPI_DBDBADD(3), 0, MPI_COMM_WORLD);

    double sum;
    if (rank == 0) {
        sum = binned_ddbconv(3, isum);
    }

    MPI_Bcast(&sum, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    return sum;
}
