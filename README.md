# Reproducible MPI_Allreduce

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
