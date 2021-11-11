# Reproducible MPI_Allreduce


## Usage

```
$ /RADTree --help
Compute a sum of distributed double values
Usage:
  RADTree [OPTION...]

      --allreduce        Use MPI_Allreduce to compute the sum
      --baseline         Gather numbers on a single rank and use
                         std::accumulate
      --tree             Use the distributed binary tree scheme to compute
                         the sum (default: true)
  -f, --file arg         File name of the binary psllh file
  -r, --repetitions arg  Repeat the calculation at most n times (default:
                         18446744073709551615)
  -d, --duration arg     Run the calculation for at least n seconds.
                         (default: 0)
  -h, --help             Display this help message
```

`RADTree` allows summation of per-site log-likelihood files (psllh, binpsllh).
There are three methods of calculation:
1. *Allreduce*. The summands are scattered across the cluster, accumulated locally and then reduced into a global sum.
Since the order of summation depends on the cluster size, this method yields different results on different clusters.
2. *Baseline*. This is a simple variant used to compare performance of the tree summation. It simply gathers all summands
on a single rank, accumulates them locally and broadcasts the result.
3. *Tree*. The summands are distributed across the cluster. The summation order is given through a binary tree, with
each leaf corresponding to a summand. The ranks reduce the sums locally in the given order and exchange intermediary 
results where necessary. After the whole sum has been computed as root of the tree, rank 0 broadcasts it to the other ranks.

## Build

```sh
cmake -DBUILD_TESTS=ON -DBUILD_BENCHMARKS=ON -B build -S .
make -C build
```


## Run tests

```sh
./build/test/tests                      # Run unit tests of C++ code
python3 test/reproducibility_test.py    # Run reproducibility tests
```

## Run benchmarks

```sh
./build/benchmarks/benchmark        # Run microbenchmarks
python3 benchmarks/benchmark.py     # Run large benchmark on different datasets
```

This will generate a sqlite database under benchmarks/results.db.
