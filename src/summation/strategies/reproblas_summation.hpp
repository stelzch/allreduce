#ifndef REPROBLAS_SUMMATION_HPP_
#define REPROBLAS_SUMMATION_HPP_

#include "summation_strategy.hpp"

class ReproBLASSummation : public SummationStrategy {
public:
    using SummationStrategy::SummationStrategy;
    double accumulate();

};


#endif
