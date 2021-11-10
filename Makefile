MPICXX=mpic++
CXXFLAGS=-std=c++2a -Wall -ggdb -O3 -I dependencies/

all: BinomialAllReduce
BinomialAllReduce:
	$(MPICXX) -o build/BinomialAllReduce $(CXXFLAGS) -I src src/io.cpp src/main.cpp src/strategies/allreduce_summation.cpp src/strategies/BaselineSummation.cpp src/strategies/binary_tree.cpp src/strategies/summation_strategy.cpp src/util.cpp

