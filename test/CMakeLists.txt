cmake_minimum_required(VERSION 3.14)

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(tests
    hello_testing.cpp
    io_tests.cpp
    binary_tree_tests.cpp
    util_tests.cpp
)
target_link_libraries(tests PRIVATE MPI::MPI_C Summation gtest_main)
include(GoogleTest)
gtest_discover_tests(tests)
