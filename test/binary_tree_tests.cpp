#include <gtest/gtest.h>
#include <vector>
#include <cmath>
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
    const int n = 30;

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
