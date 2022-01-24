#!/bin/sh
ml load compiler/gnu/11.1
ml load mpi/openmpi/4.1
ml load devel/python/3.10.0_gnu_11.1
python3 benchmarks/benchmark.py --executable="build/src/RADTree" \
	--cluster-mode \
	--description="bwUniCluster error handling" \
	--flags="--verbose" \
	--repetitions="100" \
	--cluster_size=$SLURM_NTASKS
