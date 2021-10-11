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

using namespace std;
using namespace std::string_literals;

void attach_debugger(bool condition) {
    if (!condition) return;
    bool attached = false;

    cout << "Waiting for debugger to be attached, PID: " << getpid() << endl;
    while (!attached) sleep(1);
}

class DistributedBinaryTree {
public:
    DistributedBinaryTree(uint64_t rank, vector<uint64_t> n_summands) {
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
    void read_from_array(double *array) {
        for (uint64_t i = begin; i < end; i++) {
            summands[i - begin] = array[i];
        }
    }

    uint64_t parent(uint64_t i) {
        if (i == 0) {
            throw logic_error("Node with index 0 has no parent");
        }

        // clear least significand set bit
        return i & (i - 1);
    }

    // TODO: replace with memory-efficient generator or co-routine
    vector<uint64_t> children(uint64_t i) {
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

    bool isLocal(uint64_t index) {
        return (index >= begin && index < end);
    }

    /** Determine which rank has the number with a given index */
    uint64_t rankFromIndex(uint64_t index) {
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

    double acquireNumber(uint64_t index) {
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
    vector<uint64_t> rankIntersectingSummands(void) {
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
    double accumulate(void) {
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

    double allreduce(void) {
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

protected:
    double accumulate(uint64_t index) {
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

private:
    uint64_t size, globalSize, rank, n_ranks, begin, end;
    vector<uint64_t> n_summands;
    vector<double> summands;

};

int main(int argc, char **argv) {

    if (argc < 2 || argc > 3) {
        cerr << "Usage: " << argv[0] << " <psllh> [--serial]" << endl;
        return -1;
    }


    MPI_Init(&argc, &argv);


    int c_rank, c_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &c_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &c_size);

    vector<double> summands = IO::read_psllh(argv[1]);
    assert(c_size < summands.size());
    uint64_t n_summands_per_rank = floor(summands.size() / c_size);
    uint64_t remaining = summands.size() - n_summands_per_rank * c_size;

    vector<uint64_t> summands_per_rank;
    for (uint64_t i = 0; i < c_size; i++) {
        summands_per_rank.push_back(n_summands_per_rank);
    }
    //summands_per_rank[0] += remaining;


    if (argc == 3 && (0 == strcmp(argv[2], "--serial"))) {
        if (c_rank == 0) {
            cout << "Serial Sum: " << std::accumulate(summands.begin(),
                    summands.end(), 0.0) << endl;
        }
    } else if (argc == 3 && (0 == strcmp(argv[2], "--mpi"))) {
        DistributedBinaryTree tree(c_rank, summands_per_rank);
        tree.read_from_array(&summands[0]);
        auto sum = tree.allreduce();
        if (1 || c_rank == 0) {
            cout << "MPI_Allreduce Sum: " << sum << endl;
        }
    } else {
        DistributedBinaryTree tree(c_rank, summands_per_rank);
        tree.read_from_array(&summands[0]);

        double result;
        result = tree.accumulate();

        if (c_rank == 0) {
            cout << "Distributed Sum: " << result << endl;
        }
    }


    MPI_Finalize();

    return 0;
}
