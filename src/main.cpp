#include <numeric>
#include <iostream>
#include <mpi.h>
#include <vector>
#include <cassert>
#include <cmath>
#include <memory>
#include <cmath>
#include <string>
#include <chrono>
#include <strategies/allreduce_summation.h>
#include <strategies/BaselineSummation.h>
#include "strategies/binary_tree.hpp"
#include "io.hpp"
#include "util.hpp"

using namespace std;

extern void attach_debugger(bool condition);

int repetitions = 1;

int c_rank, c_size;

void output_result(double sum) {
    printf("sum=%.64f\n", sum);
}

template<typename T> void broadcast_vector(vector<T> &src, int root) {
    MPI_Datatype type;
    MPI_Type_match_size(MPI_TYPECLASS_INTEGER, sizeof(T), &type);

    size_t vectorSize = src.size();
    MPI_Bcast(&vectorSize, 1, type, root, MPI_COMM_WORLD);

    src.resize(vectorSize);

    MPI_Bcast(&src[0], vectorSize, type,
              root, MPI_COMM_WORLD);
}

enum SummationStrategies {
    ALLREDUCE,
    BASELINE,
    TREE
};

enum SummationStrategies parse_mode_arg(string arg) {
    if (arg == "--allreduce") {
        return ALLREDUCE;
    } else if (arg == "--baseline") {
        return BASELINE;
    } else if (arg == "--tree") {
        return TREE;
    } else {
        throw runtime_error("Invalid mode: " + arg);
    }
}





int main(int argc, char **argv) {

    if (argc < 3 || string(argv[1]) == "--help") {
        cerr << "Usage: " << argv[0] << " <psllh> [--allreduce|--baseline|--tree] [repetitions]" << endl;
        return -1;
    }

    if (argc == 4) {
        repetitions = std::atoi(argv[3]);
    }

    if (repetitions < 0) {
        cerr << "Error: repetitions must be positive number" << endl;
        return -1;
    }

    SummationStrategies strategy_type = parse_mode_arg(string(argv[2]));

    MPI_Init(&argc, &argv);


    MPI_Comm_rank(MPI_COMM_WORLD, &c_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &c_size);

    vector<double> summands;
    vector<int> summands_per_rank;
    if (c_rank == 0) {

        string filename(argv[1]);
        if (filename.ends_with("binpsllh")) {
            summands = IO::read_binpsllh(filename);
        } else {
            summands = IO::read_psllh(filename);
        }
        assert(static_cast<long unsigned>(c_size) < summands.size());
        int n_summands_per_rank = floor(summands.size() / c_size);
        int remaining = summands.size() - n_summands_per_rank * c_size;
        cout << "[IO] Loaded " << summands.size() << " summands from " << filename << endl;

        for (uint64_t i = 0; i < static_cast<long unsigned>(c_size); i++) {
            summands_per_rank.push_back(n_summands_per_rank);
        }
        summands_per_rank[0] += remaining;
    }
    broadcast_vector(summands_per_rank, 0);

    std::unique_ptr<SummationStrategy> strategy;

    switch(strategy_type) {
        case ALLREDUCE:
            strategy = std::make_unique<AllreduceSummation>(c_rank, summands_per_rank);
            break;
        case BASELINE:
            strategy = std::make_unique<BaselineSummation>(c_rank, summands_per_rank);
            break;
        case TREE:
            strategy = std::make_unique<BinaryTreeSummation>(c_rank, summands_per_rank);
            break;
    }



    strategy->distribute(summands);

    double result;

    // Duration of the accumulate operation in nanoseconds
    std::vector<double> timings;
    timings.reserve(repetitions);

    for(int i = 0; i < repetitions; i++) {
        // perform the actual calculation
        auto timestamp_before = std::chrono::high_resolution_clock::now();
        result = strategy->accumulate();
        auto timestamp_after = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>
            (timestamp_after - timestamp_before);
        timings.push_back(duration.count());
        MPI_Barrier(MPI_COMM_WORLD);
    }

#ifdef REPORT_TIMINGS
    cout << "Timings";
    for (auto t : timings)
        cout << " " << t;
    cout << endl;
#endif

    if (c_rank == 0 && repetitions != 0) {
        output_result(result);

        // Calculate and output timing information
        auto avg = Util::average(timings);
        auto stddev = Util::stddev(timings);
        cout << "avg=" << avg << endl;
        cout << "stddev=" << stddev << endl;
    }

    MPI_Finalize();

    return 0;
}
