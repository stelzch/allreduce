# Reproducible MPI_Allreduce


## Usage

```
$ /BinomialAllReduce --help
Usage: ./BinomialAllReduce <psllh> [--allreduce|--baseline|--tree]
```

The `BinomialAllReduce` allows summation of per-site log-likelihood files (psllh, binpsllh).
There are three methods of calculation:
1. *Allreduce*. The summands are scattered across the cluster, accumulated locally and then reduced into a global sum.
Since the order of summation depends on the cluster size, this method yields different results on different clusters.
2. *Baseline*. This is a simple variant used to compare performance of the tree summation. It simply gathers all summands
on a single rank, accumulates them locally and broadcasts the result.
3. *Tree*. The summands are distributed across the cluster. The summation order is given through a binary tree, with
each leaf corresponding to a summand. The ranks reduce the sums locally in the given order and exchange intermediary 
results where necessary. After the whole sum has been computed as root of the tree, rank 0 broadcasts it to the other ranks.

## Running tests

```sh
mkdir build && cd build
cmake ..
make
./tests  # Run unit tests of C++ code
cd .. && python3 test/reproducibility_test.py # Run reproducibility tests
```

## Generate benchmark results

```sh
contrib/benchmark.py
```

This will generate a sqlite database under benchmarks/results.db.
