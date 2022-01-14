add_subdirectory(summation)

add_executable(RADTree main.cpp)
target_include_directories(RADTree PRIVATE . ../dependencies)
target_link_libraries(RADTree PRIVATE Summation)

#add_executable(TreeSimulations tree_simulations.cpp)
#target_link_libraries(TreeSimulations Summation)

add_executable(CriticalPath critical_path.cpp)
add_definitions(-DCRITICAL_PATH_MAIN)
target_link_libraries(CriticalPath Summation)
