#include <numeric>
#include <iostream>
#include <mpi.h>
#include <vector>
#include <cassert>
#include <cmath>
#include <string>
#include "binary_tree.hpp"
#include "io.hpp"

using namespace std;

extern void attach_debugger(bool condition);

void output_result(double sum) {
    printf("sum=%.64f\n", sum);
}


int main(int argc, char **argv) {

    if (argc < 2 || argc > 3) {
        cerr << "Usage: " << argv[0] << " <psllh> [--serial]" << endl;
        return -1;
    }


    MPI_Init(&argc, &argv);


    int c_rank, c_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &c_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &c_size);

    vector<double> summands;
    string filename(argv[1]);
    if (filename.ends_with("binpsllh")) {
        summands = IO::read_binpsllh(filename);
    } else {
        summands = IO::read_psllh(filename);
    }
    assert(c_size < summands.size());
    uint64_t n_summands_per_rank = floor(summands.size() / c_size);
    uint64_t remaining = summands.size() - n_summands_per_rank * c_size;
    cout << "[IO] Loaded " << summands.size() << " summands from " << filename << endl;

    // Reduce size of summands to nearest multiple of c_size
    // summands.resize(n_summands_per_rank * c_size);

    vector<uint64_t> summands_per_rank;
    for (uint64_t i = 0; i < c_size; i++) {
        summands_per_rank.push_back(n_summands_per_rank);
    }
    summands_per_rank[0] += remaining;


    if (argc == 3 && (0 == strcmp(argv[2], "--serial"))) {
        if (c_rank == 0) {
            cout << "Calculating sum using std::accumulate" << endl;
            attach_debugger(0);
            const double sum = std::accumulate(summands.begin(), summands.end(), 0.0);
            output_result(sum);

        }
    } else if (argc == 3 && (0 == strcmp(argv[2], "--mpi"))) {
        DistributedBinaryTree tree(c_rank, summands_per_rank);
        tree.read_from_array(&summands[0]);
        auto sum = tree.allreduce();
        if (1 || c_rank == 0) {
            cout << "Calculating using MPI_Allreduce" << endl;
            output_result(sum);
        }
    } else {
        DistributedBinaryTree tree(c_rank, summands_per_rank);
        tree.read_from_array(&summands[0]);

        double sum = tree.accumulate();

        if (c_rank == 0) {
            cout << "Calculating using BinaryTree" << endl;
            output_result(sum);
        }
    }


    MPI_Finalize();

    return 0;
}
