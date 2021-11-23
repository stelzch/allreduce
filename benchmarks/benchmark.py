import os
import sqlite3
import subprocess
import glob
import re
import platform
import argparse

parser = argparse.ArgumentParser("Run benchmark")
parser.add_argument("-e", "--executable", default="./build/src/RADTree", type=str)
parser.add_argument("-np", "--cluster_size", default=os.cpu_count(), type=int)
parser.add_argument("-t", "--time_per_run", default = 30, type=float, help="Time to spend on each iteration of the benchmark")
parser.add_argument("-f", "--flags", default="", type=str, help="Additional flags to pass to the executable under test")
parser.add_argument("-d", "--description", default="", type=str, help="Annotation that will be put in result DB")

args = parser.parse_args()
executable = args.executable
datafiles = glob.glob("data/*")
cluster_size = args.cluster_size
modes = ["--tree", "--allreduce", "--baseline"]
expected_time_per_run = args.time_per_run # seconds for each benchmark execution
flags = args.flags

con = sqlite3.connect('benchmarks/results.db')
cur = con.cursor()
cur.execute("""
CREATE TABLE IF NOT EXISTS runs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    date TEXT,
    hostname TEXT,
    revision TEXT,
    cluster_size INTEGER,
    description TEXT,
    flags TEXT
);
""")
cur.execute("""
CREATE TABLE IF NOT EXISTS results (
    run_id INTEGER,
    datafile TEXT,
    n_summands INTEGER,
    repetitions INTEGER,
    mode TEXT,
    time_ns REAL,
    stddev REAL,
    output TEXT,
    FOREIGN KEY(run_id) REFERENCES runs(id) ON DELETE CASCADE
);
""")

git_result = subprocess.run(["git", "rev-parse", "HEAD"], capture_output=True)
revision = str(git_result.stdout, 'utf-8') if git_result.returncode == 0 else "NONE"
print("REVISION: ", revision)

cur.execute("INSERT INTO runs(date, hostname, revision, cluster_size, description, flags) VALUES (datetime(), ?, ?, ?, ?, ?)",
        (platform.node(), revision.strip(), cluster_size, args.description, flags))
run_id = cur.execute("SELECT MAX(ROWID) FROM runs").fetchall()[0][0]
repetitions = 100


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

    last_result = None
    for mode in modes:
        print(f"\t\tmode = {mode[2:]}")
        cmd = f"mpirun --use-hwthread-cpus -np {cluster_size} {executable} -f {datafile} {mode} -r {repetitions} {flags}"
        print(f"\t\t\t{cmd}")
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

        cur.execute('INSERT INTO results(run_id, datafile, n_summands, repetitions, mode, time_ns, stddev, output)' \
                'VALUES (?, ?, ?, ?, ?, ?, ?, ?)',
                (run_id, datafile, n_summands, repetitions, mode[2:], time, stddev, output))
        con.commit()

con.close()
