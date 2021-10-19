import subprocess
import re
import numpy as np

EXECUTABLE = "./build/BinomialAllReduce"
FLOAT_REGEX = re.compile("^(-)?[0-9]+\.[0-9]+$")


def run_with_mpi(number_of_ranks, datafile_path, mode):
    cmd = f"mpirun -np {number_of_ranks} {EXECUTABLE} {datafile_path} {mode}"
    print(f"Running {cmd}")
    result = subprocess.run(
            cmd,
            capture_output=True,
            shell=True
    )
    calculated_sum = result.stdout.decode('utf-8').split('\n')[-2]

    if not FLOAT_REGEX.match(calculated_sum):
        raise Exception("Program did not return sum as last line as expected, last line was:\n"
                + calculated_sum)
        

    return float(calculated_sum)

def check_reproducibility(datafile, mode, reproducibilityExpected):
    results = [run_with_mpi(ranks, datafile, mode) for ranks in [2, 4, 8]]
    avg = np.average(results)
    maxDeviation = np.max(np.abs(results - avg))
    
    if reproducibilityExpected:
        if maxDeviation == 0.0:
            print(f"Reproducibility {mode} [OK], results = {results}, maxDeviation = {maxDeviation}")
        else:
            print(f"Reproducibility {mode} [FAIL], results = {results}, maxDeviation = {maxDeviation}")
    else:
        print(f"{mode} [OK], results = {results}, maxDeviation = {maxDeviation}")



if __name__ == "__main__":
    datafile = "data/354.binpsllh"
    check_reproducibility(datafile, "--serial", True)
    #check_reproducibility(datafile, "--mpi", False)
    check_reproducibility(datafile, "", True)
