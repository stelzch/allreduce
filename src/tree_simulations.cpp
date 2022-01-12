#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <util.hpp>
#include <distribution.hpp>

using namespace std;

int main(int argc, char **argv) {
    if (argc != 4) {
        cout << "Usage: " << argv[0] << " <number of summands> <number of ranks> <variance>" << endl;
        return -1;
    }

    const uint64_t n = std::atoi(argv[1]);
    const uint64_t ranks = std::atoi(argv[2]);
    const float variance = std::atof(argv[3]);
    if (n == 0 || ranks == 0) {
        cerr << "Error: zero ranks or summands makes no sense." << endl;
        return -1;
    }

    auto even = Distribution::even(n, ranks);
    auto optimized = Distribution::lsb_cleared(n, ranks, variance);

    cout << "Even: "; even.printScore();
    even.printDistribution();
    cout << endl;

    cout << "Optimized: "; optimized.printScore();
    optimized.printDistribution();

    // Try to find an optimal distribution
    Distribution candidate(1,1);
    double score = INFINITY;
    double candidateVariance = 1.0;
    for (double testedVariance = 1.0; testedVariance > 0.0; testedVariance -= 0.001) {
        auto generated = Distribution::lsb_cleared(n, ranks, testedVariance);
        if (generated.score() < score) {
            candidate = generated;
            score = generated.score();
            candidateVariance = testedVariance;
        }
    }

    cout << "Best optimization with variance " << candidateVariance << endl;
    candidate.printScore(); candidate.printDistribution();
}
