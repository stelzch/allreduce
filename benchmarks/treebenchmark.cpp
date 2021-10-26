#include "binary_tree.hpp"
#include <benchmark/benchmark.h>
#include <vector>
#include <cstdint>

using std::vector;

static void BM_parent(benchmark::State& state) {
    vector<uint64_t> n_summands {7, 5, 5, 5};
    DistributedBinaryTree tree(0, n_summands);

    for (auto _ : state) {
        auto x = tree.parent(state.range(0));
    }
}
BENCHMARK(BM_parent)->Arg(2)->Arg(8)->Arg(20);

static void BM_rankIntersectingSummands(benchmark::State& state) {
    vector<uint64_t> n_summands {7, 5, 5, 5};
    DistributedBinaryTree tree(0, n_summands);

    for (auto _ : state) {
        volatile auto result = tree.rankIntersectingSummands();
    }

}
BENCHMARK(BM_rankIntersectingSummands);


BENCHMARK_MAIN();
