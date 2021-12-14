import os
import sqlite3
import argparse
import sys
import platform
import subprocess
from benchmark import grep_number

def get_n(datafile):
    if datafile.endswith('binpsllh'):
        return (os.path.getsize(datafile) - 8) / 8 # -8 for 64-bit header, 8 bytes per double
    else:
        # text format, simply count lines
        with open(datafile, 'r') as f:
            n_summands = len(f.readlines()) - 1

        return n_summands

if __name__ == '__main__':
    parser = argparse.ArgumentParser("Perform a benchmark to determine weak/strong scaling")
    parser.add_argument("--executable", default="./build/src/RADTree", type=str)
    parser.add_argument("--datafile", type=str, help="PSLLH dataset to use", required=True)
    parser.add_argument("-n", type=int, help="Cut off datafile at n summands", default=2**64)
    parser.add_argument("--strong", action="store_true", help="Keep overall workload constant")
    parser.add_argument("--weak", action="store_true", help="Keep workload per rank constant")
    parser.add_argument("--step", type=int, default=1, help="Vary processor count by this step size")
    parser.add_argument("--min", type=int, default=1, help="Minimum number of ranks")
    parser.add_argument("--max", type=int, default=os.cpu_count(), help="Maximum number of ranks")

    args = parser.parse_args()
    executable = args.executable
    datafile = args.datafile
    m_min = args.min
    m_max = args.max
    m_step = args.step
    weak = args.weak
    strong = args.strong
    n_cutoff = args.n


    if not os.path.exists(datafile):
        print("Error: cannot find datafile " + datafile)
        sys.exit(-1)

    if not (strong or weak):
        print("Error: must either supply --weak or --strong as argument")
        sys.exit(-1)

    n_datafile = get_n(datafile)

    if n_cutoff > n_datafile:
        #print("Warning: n is larger than data file")
        pass

    description = ("strong" if strong else "weak") + " scaling"

    con = sqlite3.connect('benchmarks/results.db')
    cur = con.cursor()
    git_result = subprocess.run(["git", "rev-parse", "HEAD"], capture_output=True)
    revision = str(git_result.stdout, 'utf-8') if git_result.returncode == 0 else "NONE"

    cur.execute("INSERT INTO runs(date, hostname, revision, cluster_size, description, flags) VALUES (datetime(), ?, ?, ?, ?, ?)",
            (platform.node(), revision.strip(), os.cpu_count(), description, ""))
    con.commit()
    run_id = cur.execute("SELECT MAX(ROWID) FROM runs").fetchall()[0][0]

    ms = list(range(0 if m_min == 1 else m_min, m_max + 1, m_step))
    if m_min == 1:
        ms[0] = 1

    for m in ms:
        n = int(min(n_datafile, n_cutoff))
        if weak:
            n = m * int(n / max(ms))
        print(f"n={n}, m={m}")

        opts = f"--use-hwthread-cpus -np {m}"
        mode = "--tree"
        repetitions = "100"
        flags = ""
        cmd = f"mpirun {opts} {executable} -f {datafile} {mode} -r {repetitions} {flags} 2>&1"
        print(f"\t{cmd}")
        r = subprocess.run(cmd, shell=True, capture_output=True)
        r.check_returncode()
        output = r.stdout.decode("utf-8")
        time = grep_number("avg", output)
        stddev = grep_number("stddev", output)

        cur.execute('INSERT INTO results(run_id, datafile, n_summands, repetitions, mode, time_ns, stddev, output, ranks)' \
                'VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)',
                (run_id, datafile, n, repetitions, mode[2:], time, stddev, output, m))
        con.commit()

