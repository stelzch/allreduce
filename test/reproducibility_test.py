import subprocess
import multiprocessing
import re
import sys
import glob
import numpy as np
from functools import reduce

EXECUTABLE = "./build/src/RADTree"
FLOAT_REGEX = re.compile("^(-)?[0-9]+\.[0-9]+$")


def run_with_mpi(number_of_ranks, datafile_path, mode):
    cmd = f"mpirun --use-hwthread-cpus -np {number_of_ranks} {EXECUTABLE} -f {datafile_path} {mode}"
    print(f"\tRunning {cmd}")
    result = subprocess.run(
            cmd,
            capture_output=True,
            shell=True
    )
    output = result.stdout.decode('utf-8')
    calculated_sum_match = re.search("^sum=(.+)$", output, flags=re.MULTILINE)
    sum_bytes_match = re.search("^sumBytes=(.+)$", output, flags=re.MULTILINE)

    if not FLOAT_REGEX.match(calculated_sum_match.group(1)):
        raise Exception("Program did not return sum as last line as expected")
    
        
    if None in (calculated_sum_match, sum_bytes_match):
        raise Exception("Unable to find sum in program output")

    calculated_sum = float(calculated_sum_match.group(1))
    sum_bytes = sum_bytes_match.group(1)

    return calculated_sum, sum_bytes

def check_reproducibility(datafile, mode, reproducibilityExpected):
    ranks_to_test = range(1, multiprocessing.cpu_count())
    results = [run_with_mpi(ranks, datafile, mode) for ranks in ranks_to_test]
    allEqual = reduce(lambda a, b: a and b, [results[0][1] == x[1] for x in results]) # compare sum bytes

    results = list(map(lambda x: x[0], results))
    avg = np.average(results)
    maxDeviation = np.max(np.abs(results - avg))
    
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
    datafiles = glob.glob("data/*psllh")
    for datafile in datafiles:
        #if not check_reproducibility(datafile, "--baseline", True):
        #    retcode = -1
        if not check_reproducibility(datafile, "--tree", True):
            retcode = -1
        #if not check_reproducibility(datafile, "--reproblas", True):
        #    retcode = -1
        check_reproducibility(datafile, "--kahan", False)
    sys.exit(retcode)
