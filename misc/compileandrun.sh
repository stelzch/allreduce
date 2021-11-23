#!/bin/bash
export PATH="$PATH:/software/css//nonexclusive"
mv benchmarks/.results.db benchmarks/results.db
nonexclusive git pull
nonexclusive make -C build
exclusive python3 benchmarks/benchmark.py

