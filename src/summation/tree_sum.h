#ifndef TREE_SUM_H_
#define TREE_SUM_H_

#include <mpi.h>

#ifdef __cplusplus 
#define EXTERN extern "C"
#else
#define EXTERN
#endif


EXTERN double tree_sum(double *sendBuffer, size_t sendBufferLength, MPI_Comm comm);

#endif
