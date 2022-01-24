import os
import sqlite3
import subprocess
import glob
import re
import sys
import platform
import argparse

def init_db(cur):
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
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        run_id INTEGER,
        datafile TEXT,
        n_summands INTEGER,
        repetitions INTEGER,
        mode TEXT,
        time_ns REAL,
        stddev REAL,
        output TEXT,
        ranks INTEGER,
        FOREIGN KEY(run_id) REFERENCES runs(id) ON DELETE CASCADE
    );
    """)
    cur.execute("""
    CREATE TABLE IF NOT EXISTS durations (
        result_id INTEGER,
        time_ns REAL,
        FOREIGN KEY (result_id) REFERENCES results ON DELETE CASCADE
    );
    """)
    cur.execute("""
    CREATE TABLE IF NOT EXISTS messages (
        result_id INTEGER,
        messages_sent INTEGER,
        avg_summands_per_message REAL,
        FOREIGN KEY (result_id) REFERENCES results ON DELETE CASCADE
    );
    """)

def grep_number(name, string):
    regexp = f"^{name}=([+\-0-9.e]+)$"
    m = re.search(regexp, string, flags=re.MULTILINE)
    if m == None:
        return m
    else:
        return float(m.group(1))

def grep_durations(output):
    m = re.search("^durations=([+\-0-9.e,]+)$", output, flags=re.MULTILINE)
    
    if m == None:
        return []
    
    return list(map(float, m.group(1).split(",")))



if __name__ == "__main__":
    parser = argparse.ArgumentParser("Run benchmark")
    parser.add_argument("-e", "--executable", default="./build/src/RADTree", type=str)
    parser.add_argument("-np", "--cluster_size", default=os.cpu_count(), type=int)
    parser.add_argument("-f", "--flags", default="", type=str, help="Additional flags to pass to the executable under test")
    parser.add_argument("-d", "--description", default="", type=str, help="Annotation that will be put in result DB")
    parser.add_argument("--cluster-mode", help="Detect number of nodes and processors from environment", action='store_true')
    parser.add_argument("-r", "--repetitions", default=100, help="How many accumulation cycles to perform per benchmark")

    args = parser.parse_args()
    executable = args.executable
    datafiles = glob.glob("data/*psllh")
    cluster_size = args.cluster_size

    modes = ["--tree", "--allreduce", "--reproblas"] #"--baseline", 
    flags = args.flags
    cluster_mode = args.cluster_mode
    repetitions = args.repetitions

    con = sqlite3.connect('benchmarks/results.db')
    cur = con.cursor()
    init_db(cur)

    git_result = subprocess.run(["git", "rev-parse", "HEAD"], capture_output=True)
    revision = str(git_result.stdout, 'utf-8') if git_result.returncode == 0 else "NONE"
    print("REVISION: ", revision)

    cur.execute("INSERT INTO runs(date, hostname, revision, cluster_size, description, flags) VALUES (datetime(), ?, ?, ?, ?, ?)",
            (platform.node(), revision.strip(), cluster_size, args.description, flags))
    con.commit()
    run_id = cur.execute("SELECT MAX(ROWID) FROM runs").fetchall()[0][0]


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
            cmd = ""
            program_opts = f"-f {datafile} {mode} -r {repetitions} {flags}"
            if cluster_mode:
                cmd = f"srun -n {cluster_size} {executable} {program_opts}"
            else:
                mpi_opts = f"--use-hwthread-cpus -np {cluster_size}"
                cmd = f"mpirun {mpi_opts} {executable} {program_opts} 2>&1"
            print(f"\t\t\t{cmd}")
            r = subprocess.run(cmd, shell=True, capture_output=True)
            output = r.stdout.decode("utf-8")
            try:
                r.check_returncode()
            except subprocess.CalledProcessError:
                indented_output = "\n".join(["\t\t\t" + line for line in output.split("\n")])
                print(f"\t\t\t[ERROR] {indented_output}", file=sys.stderr)
            time = grep_number("avg", output)
            stddev = grep_number("stddev", output)
            result = grep_number("sum", output)
            durations = grep_durations(output)

            if last_result is not None:
                deviation = abs(last_result - result)
                if deviation > 1e-6:
                    print(f"\t\tLarge deviation to previous run detected: {deviation}")
            last_result = result

            cur.execute('INSERT INTO results(run_id, datafile, n_summands, repetitions, mode, time_ns, stddev, output, ranks)' \
                    'VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)', # RETURNING id',
                    (run_id, datafile, n_summands, repetitions, mode[2:], time, stddev, output, cluster_size))

            cur.execute('SELECT MAX(id) FROM results')
            result_id = cur.fetchone()[0]
            cur.executemany('INSERT INTO durations (result_id, time_ns) VALUES (?, ?)',
                    [(result_id, duration) for duration in durations])
            con.commit()

    con.close()
