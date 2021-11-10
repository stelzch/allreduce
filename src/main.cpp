#include <limits>
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
#include <strategies/binary_tree.hpp>
#include <cxxopts.hpp>
#include <io.hpp>
#include <util.hpp>

using namespace std;

extern void attach_debugger(bool condition);


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
    MPI_Init(&argc, &argv);
    cxxopts::Options options("BinomialAllReduce", "Compute a sum of distributed double values");

    constexpr unsigned long MAX_REPETITIONS = std::numeric_limits<unsigned long>::max();
    options.add_options()
        ("allreduce", "Use MPI_Allreduce to compute the sum", cxxopts::value<bool>()->default_value("false"))
        ("baseline", "Gather numbers on a single rank and use std::accumulate", cxxopts::value<bool>()->default_value("false"))
        ("tree", "Use the distributed binary tree scheme to compute the sum", cxxopts::value<bool>()->default_value("true"))
        ("f,file", "File name of the binary psllh file", cxxopts::value<string>())
        ("r,repetitions", "Repeat the calculation at most n times", cxxopts::value<unsigned long>()->default_value(to_string(MAX_REPETITIONS)))
        ("d,duration", "Run the calculation for at least n seconds.", cxxopts::value<double>()->default_value("0"))
        ("h,help", "Display this help message", cxxopts::value<bool>()->default_value("false"));

    cxxopts::ParseResult result;
    try {
        result = options.parse(argc, argv);
    } catch (cxxopts::OptionException e) {
        cerr << e.what() << endl;
        cout << options.help() << endl;
        return -1;
    }

    if (result["help"].as<bool>()) {
        cout << options.help() << endl;
        return 0;
    }

    enum SummationStrategies strategy_type;
    if (result["allreduce"].as<bool>()) {
        strategy_type = ALLREDUCE;
    } else if (result["baseline"].as<bool>()) {
        strategy_type = BASELINE;
    } else if (result["tree"].as<bool>()) {
        strategy_type = TREE;
    } else {
        cerr << "Must specify at least one of --allreduce, --baseline or --tree!" << endl;
        cout << options.help() << endl;
        return -1;
    }

    string filename(result["file"].as<string>());


    MPI_Comm_rank(MPI_COMM_WORLD, &c_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &c_size);

    vector<double> summands;
    vector<int> summands_per_rank;
    if (c_rank == 0) {
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

    double sum;
    double totalDuration = 0.0;

    // Duration of the accumulate operation in nanoseconds
    std::vector<double> timings;

    const auto repetitions = result["repetitions"].as<unsigned long int>();
    unsigned long int i = 0;
    const auto maxDuration = result["duration"].as<double>() * 1e9; // in nanoseconds

    bool keepCalculating = true;

    while (keepCalculating) {
        // perform the actual calculation
        MPI_Barrier(MPI_COMM_WORLD);
        auto timestamp_before = std::chrono::high_resolution_clock::now();
        sum = strategy->accumulate();
        auto timestamp_after = std::chrono::high_resolution_clock::now();

        if (c_rank == 0) {
            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>
                (timestamp_after - timestamp_before);
            timings.push_back(duration.count());
            totalDuration += duration.count();

            if (totalDuration > maxDuration || i++ == repetitions) {
                keepCalculating = false;
            }
        }

        MPI_Bcast(&keepCalculating, 1, MPI_BYTE, 0, MPI_COMM_WORLD);

    }

    if (c_rank == 0 && repetitions != 0) {
        output_result(sum);

        // Calculate and output timing information
        auto avg = Util::average(timings);
        auto stddev = Util::stddev(timings);
        cout << "avg=" << avg << endl;
        cout << "stddev=" << stddev << endl;
        cout << "repetitions=" << i << endl;
    }

    MPI_Finalize();

    return 0;
}
