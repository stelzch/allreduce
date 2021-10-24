#!/bin/bash
# Source: https://github.com/ReStoreCpp/ReStore/blob/main/benchmark/profile-only-on-rank-0.sh

PROFILING_RANK=0
EXE_NAME="buildDebug/BinomialAllReduce"

if [ "$OMPI_COMM_WORLD_RANK" == "$PROFILING_RANK" ]; then
        #LD_PRELOAD='/usr/lib/x86_64-linux-gnu/libprofiler.so.0' CPUPROFILE="$EXE_NAME.prof" "./$EXE_NAME"
        valgrind --tool=callgrind "./$EXE_NAME" $*
else
        "./$EXE_NAME" $*
fi

