#include <cassert>
#include <chrono>
#include <cmath>
#include <cmath>
#include <cxxopts.hpp>
#include <io.hpp>
#include <iostream>
#include <limits>
#include <memory>
#include <mpi.h>
#include <numeric>
#include <strategies/allreduce_summation.hpp>
#include <strategies/baseline_summation.hpp>
#include <strategies/binary_tree.hpp>
#include <distribution.hpp>
#include <string>
#include <util.hpp>
#include <timer.hpp>
#include <vector>

using namespace std;

extern void attach_debugger(bool condition);


int c_rank = -1, c_size = -1;

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





void cli_error(const cxxopts::Options options, const string error) {
    if (c_rank != 0) return;
    cerr << "[ERROR] " << error << "\n\n";
    cout << options.help() << "\n";
}


int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &c_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &c_size);

    cxxopts::Options options("RADTree", "Compute a sum of distributed double values");

    constexpr unsigned long MAX_REPETITIONS = std::numeric_limits<unsigned long>::max();
    options.add_options()
        ("allreduce", "Use MPI_Allreduce to compute the sum", cxxopts::value<bool>()->default_value("false"))
        ("baseline", "Gather numbers on a single rank and use std::accumulate", cxxopts::value<bool>()->default_value("false"))
        ("tree", "Use the distributed binary tree scheme to compute the sum", cxxopts::value<bool>()->default_value("true"))
        ("f,file", "File name of the binary psllh file", cxxopts::value<string>())
        ("r,repetitions", "Repeat the calculation at most n times", cxxopts::value<unsigned long>()->default_value(to_string(MAX_REPETITIONS)))
        ("d,duration", "Run the calculation for at least n seconds.", cxxopts::value<double>()->default_value("0"))
        ("c,distribution", "Number distribution, can be even, optimal or optimized,<VARIANCE>. Only relevant in tree mode", cxxopts::value<string>()->default_value("even"))
        ("v,verbose", "Be more verbose about calculations", cxxopts::value<bool>()->default_value("false"))
        ("h,help", "Display this help message", cxxopts::value<bool>()->default_value("false"));

    cxxopts::ParseResult result;
    try {
        result = options.parse(argc, argv);
    } catch (cxxopts::OptionException e) {
        if (c_rank == 0) {
            cerr << e.what() << endl;
            cout << options.help() << endl;
        }
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
        cli_error(options, "Must specify at least one of --allreduce, --baseline or --tree!");
        return -1;
    }

    string distrib_mode = result["distribution"].as<string>();
    string filename;
    try {
        filename = result["file"].as<string>();
    } catch(cxxopts::option_has_no_value_exception) {
        cli_error(options, "Must specify datafile path with -f");
        return -1;
    }
    bool verbose = result["verbose"].as<bool>();


    vector<double> summands;
    vector<int> summands_per_rank;
    if (c_rank == 0) {
        if (filename.ends_with("binpsllh")) {
            summands = IO::read_binpsllh(filename);
        } else {
            summands = IO::read_psllh(filename);
        }
        assert(static_cast<long unsigned>(c_size) < summands.size());
        cout << "[IO] Loaded " << summands.size() << " summands from " << filename << endl;

        Distribution d(0,0);
        bool initialized = false;
        if (distrib_mode == "even" || strategy_type != TREE) {
            d = Distribution::even(summands.size(), c_size);
            initialized = true;
        } else if (distrib_mode == "optimal") {
            d = Distribution::optimal(summands.size(), c_size);
            initialized = true;
        } else if (distrib_mode.starts_with("manual,")) {
            d = Distribution::from_string(distrib_mode.substr(7));

            if (d.n != static_cast<uint64_t>(summands.size())
                    || d.ranks != static_cast<uint64_t>(c_size)) {
                cli_error(options, "Distribution did not match dataset or cluster");
            } else {
                initialized = true;
            }

        } else if (distrib_mode.starts_with("optimized,")) {
            double variance = std::atof(distrib_mode.substr(10).c_str());
            
            if (variance <= 0.0 || variance > 1.0) {
                cli_error(options, "Variance of optimized distribution mode must be from (0, 1]");
            }

            d = Distribution::lsb_cleared(summands.size(), c_size, variance);
            initialized = true;
        } else {
            cli_error(options, "Invalid distribution mode: " + distrib_mode);
        }

        if(initialized) {
            summands_per_rank.resize(c_size);
            for (int i = 0; i < c_size; i++)
                summands_per_rank[i] = static_cast<int>(d.nSummands[i]);

        }

        if (verbose) {
            d.printDistribution();
            d.printScore();
        }
    }

    broadcast_vector(summands_per_rank, 0);
    if (summands_per_rank.size() == 0) {
        return -1;
    }

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
