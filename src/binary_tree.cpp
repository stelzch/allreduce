#include <mpi.h>
#include <iostream>
#include <exception>
#include <vector>
#include <numeric>
#include <strings.h>
#include <cassert>
#include <cmath>
#include <unistd.h>
#include <functional>
#include "io.hpp"
#include "binary_tree.hpp"

using namespace std;
using namespace std::string_literals;

void attach_debugger(bool condition) {
    if (!condition) return;
    bool attached = false;

    cout << "Waiting for debugger to be attached, PID: " << getpid() << endl;
    while (!attached) sleep(1);
}

DistributedBinaryTree::DistributedBinaryTree(uint64_t rank, vector<uint64_t> n_summands) {
    n_ranks = n_summands.size();
    this->rank = rank;
    this->n_summands = n_summands;
    size = n_summands[rank];
    summands.resize(size);

    begin = std::accumulate(n_summands.begin(), n_summands.begin() + rank, 0);
    end = begin + size;
    globalSize =  std::accumulate(n_summands.begin(), n_summands.end(), 0);

    //cout << "Begin: " << begin << ", end: " << end << endl;
}


// Debug call to read data from array of all summands
void DistributedBinaryTree::read_from_array(double *array) {
    for (uint64_t i = begin; i < end; i++) {
        summands[i - begin] = array[i];
    }
}

uint64_t DistributedBinaryTree::parent(uint64_t i) {
    if (i == 0) {
        throw logic_error("Node with index 0 has no parent");
    }

    // clear least significand set bit
    return i & (i - 1);
}

// TODO: replace with memory-efficient generator or co-routine
vector<uint64_t> DistributedBinaryTree::children(uint64_t i) {
    vector<uint64_t> result;

    int lsb_index = ffs(i);
    if (lsb_index == 0) {
        // If no bits are set, iterate over all zeros
        lsb_index = 31;
    }

    // Iterate over all variants where one of the least significant zeros
    // has been replaced with a one.
    for (int j = 1; j < lsb_index; j++) {
        int zero_position = j - 1;
        uint64_t child_index = i | (1 << zero_position);

        if (child_index < globalSize)
            result.push_back(child_index);
    }

    return result;
}

bool DistributedBinaryTree::isLocal(uint64_t index) {
    return (index >= begin && index < end);
}

/** Determine which rank has the number with a given index */
uint64_t DistributedBinaryTree::rankFromIndex(uint64_t index) {
    uint64_t rankLocalIndex = index;

    for (uint64_t sourceRank = 0; sourceRank < n_ranks; sourceRank++) {
        if (rankLocalIndex < n_summands[sourceRank]) {
            //cout << "Idx " << index << " is on rank " << sourceRank
            //     << " with local idx " << rankLocalIndex << endl;

            return sourceRank;
        }

        rankLocalIndex -= n_summands[sourceRank];
    }


    throw logic_error("Number cannot be found on any node");
}

double DistributedBinaryTree::acquireNumber(uint64_t index) {
    if (isLocal(index)) {
        //cout << "Number is local" << endl;
        // We have that number locally
        return summands[index - begin];
    }

    uint64_t sourceRank = rankFromIndex(index);
    double result;
    //cout << "Receiving from " << sourceRank
    //    << " with tag " << index << " data " << result << endl;
    MPI_Recv(&result, 1, MPI_DOUBLE,
            sourceRank, index, MPI_COMM_WORLD, NULL);

    return result;
}

/* Calculate all rank-intersecting summands that must be sent out because
    * their parent is non-local and located on another rank
    */
vector<uint64_t> DistributedBinaryTree::rankIntersectingSummands(void) {
    vector<uint64_t> result;

    // ignore index 0
    uint64_t startIndex = (begin == 0) ? 1 : begin;
    for (uint64_t index = startIndex; index < end; index++) {
        if (!isLocal(parent(index))) {
            result.push_back(index);
        }
    }

    return result;
}

/* Sum all numbers. Will return the total sum on rank 0
    */
double DistributedBinaryTree::accumulate(void) {
    for (auto summand : rankIntersectingSummands()) {
        //cout << "Rank intersecting summand = " << summand << endl;
        double result = accumulate(summand);
        // TODO: Send out summand
        MPI_Send(&result, 1, MPI_DOUBLE,
                rankFromIndex(parent(summand)), summand, MPI_COMM_WORLD);
        //cout << "Sending to " << rankFromIndex(parent(summand))
        //    << " with tag " << summand << " data " << result << endl;
                
    }

    if (rank == 0) {
        return accumulate(0);
    } else {
        return -1.0;
    }
}

double DistributedBinaryTree::allreduce(void) {
    for (const auto x : n_summands) {
        // MPI Allreduce only works with uniform summand count
        assert(x == n_summands[0]);
    }

    const size_t size = n_summands[0];

    vector<double> reduced_sums(size);
    MPI_Allreduce(&summands[0], &reduced_sums[0], size,
            MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

    //attach_debugger(0 == rank);

    // Calculate sum of accumulated values
    auto sum = std::accumulate(reduced_sums.begin(),
            reduced_sums.end(), 0);

    return sum;
}

double DistributedBinaryTree::accumulate(uint64_t index) {
    //cout << "accumulate(" << index << ")" << endl;
    double accumulator = acquireNumber(index);

    if (!isLocal(index)) {
        //cout << endl;
        return accumulator;

    }

    int lsb_index = ffs(index);
    if (lsb_index == 0) {
        // If no bits are set, iterate over all zeros
        lsb_index = 31;
    }

    // Iterate over all variants where one of the least significant zeros
    // has been replaced with a one.
    for (int j = 1; j < lsb_index; j++) {
        int zero_position = j - 1;
        uint64_t child_index = index | (1 << zero_position);

        if (child_index < globalSize) {
            double num = accumulate(child_index);
            accumulator += num;
        }
    }

    return accumulator;
}
