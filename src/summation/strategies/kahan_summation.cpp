#include "kahan_summation.hpp"
#include <mpi.h>

double KahanSummation::accumulate() {

    /* Robey et al.: In search of numerical consistency in parallel programming (2011)
     * Listing 4
     */

    double corrected_next_term, new_sum;
    struct esum_type local, global;

    local.sum = 0.0;
    local.correction = 0.0;


    for (int i = 0; i < summands.size(); i++) {
        corrected_next_term = summands[i] + local.correction;
        new_sum = local.sum + corrected_next_term;
        local.correction = corrected_next_term - (new_sum - local.sum);
        local.sum = new_sum;
    }

    MPI_Allreduce(&local, &global, 2, MPI_DOUBLE, MPI_SUM, comm);
    return global.sum;
}
