#include "strategies/binary_tree.hpp"
#include <benchmark/benchmark.h>
#include <vector>
#include <numeric>
#include <cstdint>
#include <mpi.h>
#include <chrono>
#include <iostream>
#include <random>
#include <immintrin.h>

using std::cout;
using std::endl;
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

static void BM_chrono(benchmark::State& state) {
    using ns = std::chrono::nanoseconds;
    std::chrono::duration<double> acc;
    for (auto _ : state) {
        volatile auto t1 = std::chrono::high_resolution_clock::now();
        //auto t2 = std::chrono::high_resolution_clock::now();
        //auto diff = (t2 - t1);
        //acc += diff;

    }
}
BENCHMARK(BM_chrono);

static void BM_iterative(benchmark::State& state) {
    const int n = state.range(0);

    // Prepare input data
    vector<double> data;
    data.reserve(n);
    for(int i = 0; i < n; i++) data.push_back(i);

    vector<int> n_summands = {n};
    BinaryTreeSummation tree(0, n_summands);
    tree.distribute(data);

    for (auto _ : state) {
       tree.accumulate(0); 
    }
}
BENCHMARK(BM_iterative)->RangeMultiplier(8)->Range(1, 1 << 27);

static void BM_noCheckIterative(benchmark::State& state) {
    const int n = state.range(0);

    // Prepare input data
    vector<double> data;
    data.reserve(n);
    for(int i = 0; i < n; i++) data.push_back(i);

    vector<int> n_summands = {n};
    BinaryTreeSummation tree(0, n_summands);
    tree.distribute(data);

    for (auto _ : state) {
       volatile double a = tree.nocheckAccumulate(); 
    }
}
BENCHMARK(BM_noCheckIterative)->RangeMultiplier(8)->Range(1, 1 << 27);

static void BM_recursive(benchmark::State& state) {
    const int n = state.range(0);

    // Prepare input data
    vector<double> data;
    data.reserve(n);
    for(int i = 0; i < n; i++) data.push_back(i);


    vector<int> n_summands = { n };
    BinaryTreeSummation tree(0, n_summands);
    tree.distribute(data);

    for (auto _ : state) {
       tree.recursiveAccumulate(0); 
    }
}
BENCHMARK(BM_recursive)->RangeMultiplier(8)->Range(1, 1 << 27);

static void BM_accumulative(benchmark::State& state) {
    const int n = state.range(0);

    // Prepare input data
    vector<double> data;
    data.reserve(n);
    for(int i = 0; i < n; i++) data.push_back(i);

    for (auto _ : state) {
        volatile double result = std::accumulate(data.begin(), data.end(), 0.0);
    }
}
BENCHMARK(BM_accumulative)->RangeMultiplier(8)->Range(1, 1 << 27);


__attribute__((optimize("O3"))) static void BM_avxsubtree8(benchmark::State& state) {
    const size_t n = 1 * 256 * 1024 * 1024 / sizeof(double);
    vector<double> results(n);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> distrib(-1e14,1e14);

    for (int i = 0; i < n; i++)
        results[i] = distrib(gen);

    volatile double result;

    for (auto _ : state) {
        for (size_t i = 0; i + 8 < n; i += 8) {
            __m256d a = _mm256_loadu_pd(static_cast<double *>(&results[i]));
            __m256d b = _mm256_loadu_pd(static_cast<double *>(&results[i + 4]));
            __m256d level1Sum = _mm256_hadd_pd(a, b);

            __m128d c = _mm256_extractf128_pd(level1Sum, 1); // Fetch upper 128bit
            __m128d d = _mm256_castpd256_pd128(level1Sum); // Fetch lower 128bit
            __m128d level2Sum = _mm_add_pd(c, d);

            __m128d level3Sum = _mm_hadd_pd(level2Sum, level2Sum);

            results[i / 8] = _mm_cvtsd_f64(level3Sum);
        }
    }

}
BENCHMARK(BM_avxsubtree8);

__attribute__((optimize("O3"))) static void BM_subtree8(benchmark::State& state) {
    const size_t n = 1 * 256 * 1024 * 1024 / sizeof(double);
    assert(n % 8 == 0);
    vector<double> results(n);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> distrib(-1e14,1e14);

    for (int i = 0; i < n; i++)
        results[i] = distrib(gen);

    volatile double result;

    uint64_t counter = 0;

    for (auto _ : state) {

        for (int y = 0; y < 3; y++) {
            const size_t stride = (1L << y);
            size_t elementsWritten = 0;

            for (size_t i = 0; elementsWritten < (n / 2 / stride); i += 2 * stride) {
                results[elementsWritten++] = results[i] + results[i + stride];
            }

            size_t expected;
            if (y == 0) {
                expected = n / 2;
            } else if (y == 1) {
                expected = n / 4;
            } else if (y == 2) {
                expected = n / 8;
            }

            //printf("%li ?= %li\n", expected, elementsWritten);
            //assert(expected == elementsWritten);
        }

    }

}
BENCHMARK(BM_subtree8);


BENCHMARK_MAIN();
