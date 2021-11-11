//
// Created by christoph on 28.10.21.
//

#ifndef BINOMIALALLREDUCE_BASELINESUMMATION_H
#define BINOMIALALLREDUCE_BASELINESUMMATION_H

#include "summation_strategy.hpp"

class BaselineSummation : public SummationStrategy {
public:
    using SummationStrategy::SummationStrategy;
    double accumulate();

};


#endif //BINOMIALALLREDUCE_BASELINESUMMATION_H
