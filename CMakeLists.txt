cmake_minimum_required(VERSION 3.10)

# Project properties
project(RADTree)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# Dependencies on system libraries
find_package(MPI REQUIRED)
include_directories(SYSTEM ${MPI_INCLUDE_PATH})

# Enable optimizations
add_compile_options(-Wall -O3 -ggdb -mfpmath=sse -mmmx -msse -msse2)

# Parts of the project
add_subdirectory(src)

if(BUILD_TESTS)
    add_subdirectory(test)
endif()

if(BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()
