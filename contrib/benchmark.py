import os
import sqlite3
import subprocess
import glob
import re

datafiles = glob.glob("data/*")
cluster_sizes = [4, 8]
modes = ["--tree", "--mpi", "--serial"][:2]
n_runs = 1
program_repetitions = 1.0e4

os.remove('benchmarks/results.db')
con = sqlite3.connect('benchmarks/results.db')
cur = con.cursor()
cur.execute("""CREATE TABLE results (
    datafile TEXT,
    n_summands INTEGER,
    cluster_size INTEGER,
    mode TEXT,
    run INTEGER,
    rank INTEGER,
    time_us REAL);
""")

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
        for mode in modes:
            print(f"\t\tmode = {mode[2:]}")
            for run in range(n_runs):
                print(f"\t\t\t{run + 1} / {n_runs}")
                cmd = f"mpirun -np {cluster_size} ./build/BinomialAllReduce {datafile} {mode}"
                r = subprocess.run(cmd, shell=True, capture_output=True)
                output = r.stdout.decode("utf-8")
                rankTimes = [(int(match.group(1)), float(match.group(2)))
                        for match in re.finditer(r"Calculation on rank (\d+) took (\S+) µs$",
                            output,
                            flags=re.MULTILINE)]
                for (rank, time) in rankTimes:
                    cur.execute('INSERT INTO results VALUES (?, ?, ?, ?, ?, ?, ?)',
                            (datafile, n_summands, cluster_size, mode[2:], run, rank, time / program_repetitions))
                    con.commit()

con.close()
