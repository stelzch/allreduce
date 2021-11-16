add_library(Summation strategies/summation_strategy.cpp
                        strategies/binary_tree.cpp
                        strategies/allreduce_summation.cpp
                        strategies/baseline_summation.cpp
                        io.cpp
                        util.cpp
                        distribution.cpp
)
target_include_directories(Summation PRIVATE ../../dependencies)
target_include_directories(Summation PUBLIC .)
target_link_libraries(Summation PUBLIC MPI::MPI_C MPI::MPI_CXX)
