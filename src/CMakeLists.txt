add_subdirectory(summation)

add_executable(RADTree main.cpp)
target_include_directories(RADTree PRIVATE . ../dependencies)
target_link_libraries(RADTree Summation)

add_executable(TreeSimulations tree_simulations.cpp)
target_link_libraries(TreeSimulations Summation)
