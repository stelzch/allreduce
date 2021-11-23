#include <gtest/gtest.h>
#include <vector>
#include <util.hpp>

using std::vector;

TEST(UtilTests, MeanAndStdDev) {
    /* Generate samples with the following python oneliner:
     * python3 -c 'import random;import numpy as np;x=[(random.random() - 0.5) * 200 for _ in range(10)];print("{" + ", ".join(map(str, x))+ "}");print("avg =",np.mean(x));print("stddev = ", np.std(x))'
     */

    const vector<double> x1 {74.03713603863244, 8.131492852275457, 18.06072594094492, 5.479708476154999, -3.987994044752985, 88.36779581877816, -2.329568968024498, 74.44785074578665, -69.32078675688773, -11.472387132627503};
    EXPECT_NEAR(Util::average(x1), 18.141397297027986, 1e-9);
    EXPECT_NEAR(Util::stddev(x1), 45.74101081759096, 1e-9);

    const vector<double> x2 {-48.41834353038252, 6.8214935874101545, 89.47272847134407, 92.55766258811397, 5.194230174948022, 38.42784022206398, -74.39749890482281, -37.62633059467648, 73.73591982461465, -95.52601021479674};
    EXPECT_NEAR(Util::average(x2), 5.0241691623816305, 1e-9);
    EXPECT_NEAR(Util::stddev(x2), 64.59670895614128, 1e-9);
}

TEST(UtilTests, Zip) {
    const vector<double> a = {1.0, 2.0, 3.0};
    const vector<int> b = {1, 0, 1, 0, 1};

    const vector<double> expected = {1.0, 0.0, 3.0};

    int count = 0;
    Util::zip(a.begin(), a.end(), b.begin(), b.end(), [&count, expected](const double &a, const int &b) {
        EXPECT_EQ(a * b, expected[count++]);
    });
}
