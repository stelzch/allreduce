#include "tree_sum.h"
#include "strategies/binary_tree.hpp"


extern "C" {

double tree_sum(double *sendBuffer, size_t sendBufferLength, MPI_Comm comm) {
    return BinaryTreeSummation::global_sum(sendBuffer, sendBufferLength, comm);
}

}
