cmake_minimum_required(VERSION 3.14)
project(BinomialAllReduce)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)


enable_testing()
add_library(IO src/io.cpp)
add_library(Summation src/strategies/binary_tree.cpp src/strategies/summation_strategy.cpp src/strategies/summation_strategy.cpp src/strategies/allreduce_summation.cpp src/strategies/allreduce_summation.h src/strategies/BaselineSummation.cpp src/strategies/BaselineSummation.h src/data_distribution.hpp)
add_library(Util src/util.cpp)
include_directories(src)
find_package(MPI REQUIRED)
add_executable(
    tests
    test/hello_testing.cpp
    test/io_tests.cpp
    test/binary_tree_tests.cpp
)
target_include_directories(tests PRIVATE ${MPI_CXX_INCLUDE_PATH})
target_compile_options(tests PRIVATE ${MPI_CXX_COMPILE_FLAGS})
target_link_libraries(tests ${MPI_CXX_LIBRARIES} ${MPI_CXX_LINK_FLAGS} IO Summation)
add_custom_command(TARGET tests PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/test/data $<TARGET_FILE_DIR:tests>/test/data)
target_link_libraries(
    tests
    gtest_main
)
include(GoogleTest)
gtest_discover_tests(tests)

find_package(benchmark REQUIRED)
add_executable(
    benchmark
    benchmarks/treebenchmark.cpp
)
target_include_directories(benchmark PRIVATE ${MPI_CXX_INCLUDE_PATH})
target_compile_options(benchmark PRIVATE ${MPI_CXX_COMPILE_FLAGS})
target_link_libraries(benchmark ${MPI_CXX_LIBRARIES} ${MPI_CXX_LINK_FLAGS} Summation benchmark::benchmark)


add_executable(tree_simulation src/tree_simulations.cpp)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -ggdb -O3")
target_link_libraries(tree_simulation Util)


find_package(MPI REQUIRED)
add_executable(
    BinomialAllReduce
    src/main.cpp
)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -ggdb -O3")
target_include_directories(BinomialAllReduce PRIVATE ${MPI_CXX_INCLUDE_PATH})
target_compile_options(BinomialAllReduce PRIVATE ${MPI_CXX_COMPILE_FLAGS})
target_link_libraries(BinomialAllReduce ${MPI_CXX_LIBRARIES} ${MPI_CXX_LINK_FLAGS} IO Summation Util)
