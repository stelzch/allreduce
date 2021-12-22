#include <gtest/gtest.h>
#include <distribution.hpp>

TEST(DistributionTests, RoundingUp) {
    EXPECT_EQ(Distribution::roundUp(23), 24);
    EXPECT_EQ(Distribution::roundUp(24), 32);

    EXPECT_EQ(Distribution::roundUp(1), 2);
    EXPECT_EQ(Distribution::roundUp(2), 4);
}
