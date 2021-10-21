import subprocess
import multiprocessing
import re
import sys
import numpy as np
from functools import reduce

EXECUTABLE = "./build/BinomialAllReduce"
FLOAT_REGEX = re.compile("^(-)?[0-9]+\.[0-9]+$")


def run_with_mpi(number_of_ranks, datafile_path, mode):
    cmd = f"mpirun -np {number_of_ranks} {EXECUTABLE} {datafile_path} {mode}"
    print(f"\tRunning {cmd}")
    result = subprocess.run(
            cmd,
            capture_output=True,
            shell=True
    )
    calculated_sum = next(filter(lambda l: l.startswith('sum='), result.stdout.decode('utf-8').split('\n')))[4:]

    if not FLOAT_REGEX.match(calculated_sum):
        raise Exception("Program did not return sum as last line as expected, last line was:\n"
                + calculated_sum)
        

    return float(calculated_sum)

def check_reproducibility(datafile, mode, reproducibilityExpected):
    ranks_to_test = range(1, multiprocessing.cpu_count())
    results = [run_with_mpi(ranks, datafile, mode) for ranks in ranks_to_test]
    avg = np.average(results)
    maxDeviation = np.max(np.abs(results - avg))
    allEqual = reduce(lambda a, b: a and b, [results[0] == x for x in results])
    
    if reproducibilityExpected:
        if allEqual:
            print(f"Reproducibility {mode} [OK], results = {results}")
            return True
        else:
            print(f"Reproducibility {mode} [FAIL], results = {results}, maxDeviation = {maxDeviation}")
            return False
    else:
        msg = "OK" if allEqual else "FAIL"
        print(f"Reproducibility {mode} [{msg}], results = {results}, maxDeviation = {maxDeviation}")
        return True



if __name__ == "__main__":
    retcode = 0
    for file in ["354.binpsllh", "fusob.psllh", "prim.psllh", "XiD4.psllh"]:
        datafile = "data/" + file
        if not check_reproducibility(datafile, "--serial", True):
            retcode = -1
        if not check_reproducibility(datafile, "--tree", True):
            retcode = -1
        check_reproducibility(datafile, "--mpi", False)
    sys.exit(retcode)
