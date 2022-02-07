#ifndef TREE_SUM_H_
#define TREE_SUM_H_

#include <mpi.h>


extern "C" double tree_sum(double *sendBuffer, size_t sendBufferLength, MPI_Comm comm);

#endif
