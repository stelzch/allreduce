#include "strategies/binary_tree.hpp"
#include <benchmark/benchmark.h>
#include <vector>
#include <cstdint>
#include <mpi.h>

using std::vector;

static void BM_parent(benchmark::State& state) {
    vector<int> n_summands {7, 5, 5, 5};
    BinaryTreeSummation tree(0, n_summands);

    for (auto _ : state) {
        auto x = tree.parent(state.range(0));
    }
}
BENCHMARK(BM_parent)->Arg(2)->Arg(8)->Arg(20);

static void BM_rankIntersectingSummands(benchmark::State& state) {
    const auto n = 21410970; // Tarv D7
    const auto m = 256; // 256 cores
    const auto nPerRank = n / m;
    const auto remainder = n % m;

    vector<int> n_summands(m);
    for (int i = 0; i < m; i++)
        n_summands.push_back(nPerRank + ((i < remainder) ? 1 : 0));

    BinaryTreeSummation tree(state.range(0), n_summands);

    for (auto _ : state) {
        volatile auto result = tree.calculateRankIntersectingSummands();
    }

}
BENCHMARK(BM_rankIntersectingSummands)->Arg(0)->Arg(121)->Arg(9)->Arg(255);

static void BM_summation(benchmark::State& state) {
    volatile double a = 0.0;
    for (auto _ : state) {
        a += 1.23543;
    }

}
BENCHMARK(BM_summation);

BENCHMARK_MAIN();
