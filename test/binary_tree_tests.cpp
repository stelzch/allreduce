#include <random>
#include <gtest/gtest.h>
#include <vector>
#include <cmath>
#include <distribution.hpp>
#include "strategies/binary_tree.hpp"

using std::vector;

TEST(BinaryTreeTests, BinaryTreeStructure) {
    vector<int> n_summands { 3, 2, 4 };
    BinaryTreeSummation tree(0, n_summands);

    // 8 summands, we are the first rank which has 3 summands.
    // The first three should be local then, and the others remote
    for (int i = 0; i < 8; i++) {
        if (i < 3) {
            EXPECT_TRUE(tree.isLocal(i));
        } else {
            EXPECT_FALSE(tree.isLocal(i));
        }
    }

    EXPECT_EQ(tree.rankFromIndex(4), 1);
    EXPECT_EQ(tree.rankFromIndex(6), 2);

    EXPECT_EQ(tree.parent(5), 4);
    EXPECT_EQ(tree.parent(4), 0);
    EXPECT_EQ(tree.parent(2), 0);

    EXPECT_EQ(tree.calculateRankIntersectingSummands().size(), 0);

    BinaryTreeSummation treeRk1 (1, n_summands);
    auto rankIntersectingSummands = treeRk1.calculateRankIntersectingSummands();
    EXPECT_EQ(rankIntersectingSummands.size(), 2);
    EXPECT_EQ(rankIntersectingSummands[0], 3);
    EXPECT_EQ(rankIntersectingSummands[1], 4);
}

TEST(BinaryTreeTests, IterativeEqualToRecursive) {
    vector<int> dataSizes;

    // Perform 10 tests with random data sizes
    for (int i = 0; i < 10; i++) {
        dataSizes.push_back((std::rand() % 160'000) + 1);
    }

    for (auto n : dataSizes) {
        vector<int> n_summands { n };
        vector<double> numbers(n);

        // Generate test data
        for (int i = 0; i < n; i++) {
            numbers[i] = exp2(i);
        }

        BinaryTreeSummation tree(0, n_summands);
        tree.distribute(numbers);

        EXPECT_EQ(tree.accumulate(0), tree.recursiveAccumulate(0));
    }
}

TEST(BinaryTreeTests, rankFromIndex) {
    const size_t n = 16 * 1024 * 1024 / sizeof(double);
    const int m = 3;

    vector<int> nSummands;
    for (int i = 0; i < m; i++) {
        nSummands.push_back(n / m);
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, n - 1);

    BinaryTreeSummation tree(0, nSummands);

    for (int i = 0; i < 1000; i++) {
        int idx = distrib(gen);

        EXPECT_EQ(tree.rankFromIndexMap(idx),
                tree.rankFromIndex(idx));
    }
}

TEST(BinaryTreeTests, rankFromIndexClosedForm) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> n_distrib(1, 500);
    std::uniform_int_distribution<> m_distrib(8, 128);

    for (auto i = 0UL; i < 100; i++) {
        auto d = Distribution::even_remainder_on_last(n_distrib(gen), m_distrib(gen));
        vector<int> nSummands;
        nSummands.reserve(d.ranks);
        for (auto x : d.nSummands) nSummands.push_back(x);

        BinaryTreeSummation s(0, nSummands, MPI_COMM_WORLD);

        std::uniform_int_distribution<> i_distrib(0, d.n - 1);
        for (auto j = 0UL; j < 10000; j++) {
            auto index = i_distrib(gen);

            EXPECT_EQ(s.rankFromIndexMap(index), s.rankFromIndexClosedForm(index)) << "n = " << d.n << " m = " << d.ranks << " i = " << index;

        }
    }
}
