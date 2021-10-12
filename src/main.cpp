#include <numeric>
#include <iostream>
#include <mpi.h>
#include <vector>
#include <cassert>
#include <cmath>
#include "binary_tree.hpp"
#include "io.hpp"

using namespace std;

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

    summands.resize(n_summands_per_rank * c_rank);

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
