set(CMAKE_CXX_STANDARD 20)

include_directories(SYSTEM ${MPI_INCLUDE_PATH})

# Add ReproBLAS dependency
set(REPROBLAS_PATH ${PROJECT_SOURCE_DIR}/dependencies/ReproBLAS)
set(REPROBLAS_BUILD ${REPROBLAS_PATH}/build/release/Linux-x86_64)

add_library(reproBLAS STATIC IMPORTED)
add_library(binned STATIC IMPORTED)
add_library(binnedMPI STATIC IMPORTED)
add_library(binnedBLAS STATIC IMPORTED)
set_property(TARGET reproBLAS PROPERTY
    IMPORTED_LOCATION ${REPROBLAS_BUILD}/src/reproBLAS/obj/libreproblas.a)
set_property(TARGET binned PROPERTY
    IMPORTED_LOCATION ${REPROBLAS_BUILD}/src/binned/obj/libbinned.a)
set_property(TARGET binnedBLAS PROPERTY
    IMPORTED_LOCATION ${REPROBLAS_BUILD}/src/binnedBLAS/obj/libbinnedblas.a)
set_property(TARGET binnedMPI PROPERTY
    IMPORTED_LOCATION ${REPROBLAS_BUILD}/src/binnedMPI/obj/libbinnedmpi.a)

message("Build DIRR is ${REPROBLAS_BUILD}")

add_library(Summation strategies/summation_strategy.cpp
                        strategies/binary_tree.cpp
                        strategies/allreduce_summation.cpp
                        strategies/baseline_summation.cpp
                        strategies/reproblas_summation.cpp
                        io.cpp
                        util.cpp
                        distribution.cpp
)
if(ENABLE_INSTRUMENTATION)
    add_definitions(-DENABLE_INSTRUMENTATION)
endif()

target_include_directories(Summation PRIVATE ../../dependencies ${REPROBLAS_PATH}/include)
target_include_directories(Summation PUBLIC .)
target_link_libraries(Summation PUBLIC MPI::MPI_C MPI::MPI_CXX
    binned binnedBLAS reproBLAS binnedMPI)
