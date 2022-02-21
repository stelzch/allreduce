#ifndef KAHAN_SUMMATION_HPP_
#define KAHAN_SUMMATION_HPP_

#include "summation_strategy.hpp"

struct esum_type {
    double sum;
    double correction;
};

class KahanSummation : public SummationStrategy {
public:
    using SummationStrategy::SummationStrategy;
    double accumulate();
};


#endif
