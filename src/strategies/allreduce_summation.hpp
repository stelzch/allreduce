//
// Created by christoph on 28.10.21.
//

#ifndef BINOMIALALLREDUCE_ALLREDUCE_SUMMATION_H
#define BINOMIALALLREDUCE_ALLREDUCE_SUMMATION_H

#include "summation_strategy.hpp"

class AllreduceSummation : public SummationStrategy {
public:
    using SummationStrategy::SummationStrategy;
    double accumulate();

};


#endif //BINOMIALALLREDUCE_ALLREDUCE_SUMMATION_H
