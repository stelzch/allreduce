import os
import sqlite3
import subprocess
import glob
import re

datafiles = glob.glob("data/*")
cluster_sizes = [os.cpu_count()]
modes = ["--tree", "--allreduce", "--baseline"]
program_repetitions = 50

os.remove('benchmarks/results.db')
con = sqlite3.connect('benchmarks/results.db')
cur = con.cursor()
cur.execute("""CREATE TABLE results (
    datafile TEXT,
    n_summands INTEGER,
    cluster_size INTEGER,
    mode TEXT,
    time_ns REAL,
    stddev REAL);
""")

def grep_number(name, string):
    regexp = f"^{name}=([+\-0-9.e]+)$"
    m = re.search(regexp, string, flags=re.MULTILINE)
    if m == None:
        return m
    else:
        return float(m.group(1))


for datafile in datafiles:
    print(datafile)

    # Determine number of summands
    n_summands = 0
    if datafile.endswith('binpsllh'):
        n_summands = (os.path.getsize(datafile) - 8) / 8 # -8 for 64-bit header, 8 bytes per double
    else:
        # text format, simply count lines
        f = open(datafile, 'r')
        n_summands = len(f.readlines()) - 1
        f.close()

    for cluster_size in cluster_sizes:
        print(f"\tnp = {cluster_size}")
        last_result = None
        for mode in modes:
            print(f"\t\tmode = {mode[2:]}")
            repetitions = program_repetitions * 1_000 if n_summands < 2**16 else program_repetitions
            cmd = f"mpirun -np {cluster_size} ./build/BinomialAllReduce {datafile} {mode} {program_repetitions}"
            r = subprocess.run(cmd, shell=True, capture_output=True)
            r.check_returncode()
            output = r.stdout.decode("utf-8")
            time = grep_number("avg", output)
            stddev = grep_number("stddev", output)
            result = grep_number("sum", output)

            if last_result is not None:
                deviation = abs(last_result - result)
                if deviation > 1e-6:
                    print(f"\t\tLarge deviation to previous run detected: {deviation}")
            last_result = result

            cur.execute('INSERT INTO results VALUES (?, ?, ?, ?, ?, ?)',
                    (datafile, n_summands, cluster_size, mode[2:], time, stddev))
            con.commit()

con.close()
