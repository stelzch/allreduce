#include <mpi.h>
#include <iostream>
#include <exception>
#include <vector>
#include <numeric>
#include <strings.h>
#include <cassert>
#include <cmath>
#include <unistd.h>

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

        cout << "Begin: " << begin << ", end: " << end << endl;
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

            if (child_index < size)
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
                cout << "Idx " << index << " is on rank " << sourceRank
                     << " with local idx " << rankLocalIndex << endl;

                return sourceRank;
            }

            rankLocalIndex -= n_summands[sourceRank];
        }


        throw logic_error("Number cannot be found on any node");
    }

    double acquireNumber(uint64_t index) {
        if (isLocal(index)) {
            cout << "Number is local" << endl;
            // We have that number locally
            return summands[index - begin];
        }

        uint64_t sourceRank = rankFromIndex(index);
        double result;
        MPI::COMM_WORLD.Recv(&result, 1, MPI_DOUBLE,
                sourceRank, index);

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
                cout << "Rank intersecting summand" << endl;
            }
        }

        return result;
    }

    /* Sum all numbers. Will return the total sum on rank 0
     */
    double accumulate(void) {
        for (auto summand : rankIntersectingSummands()) {
            cout << "Rank intersecting summand = " << summand << endl;
            double result = accumulate(summand);
            // TODO: Send out summand
            MPI::COMM_WORLD.Send(&result, 1, MPI_DOUBLE,
                    rankFromIndex(summand), summand);
                    
        }

        if (rank == 0) {
            return accumulate(0);
        } else {
            return -1.0;
        }
    }

protected:
    double accumulate(uint64_t index) {
        cout << "accumulate(" << index << ") = ";
        double accumulator = acquireNumber(index);
        cout << accumulator << " ";

        for (auto child : children(index)) {
            double num = acquireNumber(child);
            cout << "+ " << num << " ";
            accumulator += acquireNumber(child);
        }
        cout << endl;

        return accumulator;
    }

private:
    uint64_t size, rank, n_ranks, begin, end;
    vector<uint64_t> n_summands;
    vector<double> summands;
};

int main(int argc, char **argv) {
    MPI::Init(argc, argv);


    uint64_t c_rank = MPI::COMM_WORLD.Get_rank();
    uint64_t c_size = MPI::COMM_WORLD.Get_size();

    vector<double> summands = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};
    assert(c_size < summands.size());
    uint64_t n_summands_per_rank = floor(summands.size() / c_size);
    uint64_t remaining = summands.size() - n_summands_per_rank * c_size;

    vector<uint64_t> summands_per_rank;
    for (uint64_t i = 0; i < c_size; i++) {
        summands_per_rank.push_back(n_summands_per_rank);
    }
    summands_per_rank.back() += remaining;


    for (auto x : summands_per_rank) {
        cout << x << " ";
    }
    cout << endl;


    //attach_debugger(1 == c_rank);
    DistributedBinaryTree tree(c_rank, summands_per_rank);
    tree.read_from_array(&summands[0]);

    double result = tree.accumulate();

    if (c_rank == 0) {
        cout << "Sum: " << result << endl;
    }


    MPI::Finalize();

    return 0;
}

/**
 * TODO: Fix the accumulate method. It likely needs to be made recursive
 */
