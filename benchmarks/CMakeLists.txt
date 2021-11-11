find_package(benchmark REQUIRED)

add_executable(
    benchmark
    treebenchmark.cpp
)

target_link_libraries(benchmark PRIVATE MPI::MPI_C Summation benchmark::benchmark)
