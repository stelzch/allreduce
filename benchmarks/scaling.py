import shutil
import os
import sqlite3
import argparse
import sys
import platform
import zipfile
import subprocess
import glob
from benchmark import grep_number, init_db

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
    parser.add_argument("--scorep", action="store_true", help="Collect ScoreP metrics")
    parser.add_argument("--modes", type=str, help="computation modes", default="tree")
    parser.add_argument("--cluster-mode", help="For use on cluster with SLURM workload manager", action='store_true')

    args = parser.parse_args()
    executable = args.executable
    datafile = args.datafile
    m_min = args.min
    m_max = args.max
    m_step = args.step
    weak = args.weak
    strong = args.strong
    n_cutoff = args.n
    scorep = args.scorep
    modes = args.modes.split(",")
    cluster_mode = args.cluster_mode


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
    init_db(cur)
    git_result = subprocess.run(["git", "rev-parse", "HEAD"], capture_output=True)
    revision = str(git_result.stdout, 'utf-8') if git_result.returncode == 0 else "NONE"

    if cluster_mode:
        for key, value in os.environ.items():
            if key.startswith('SLURM'):
                print(f"\t{key}: {value}")

    cur.execute("INSERT INTO runs(date, hostname, revision, cluster_size, description, flags) VALUES (datetime(), ?, ?, ?, ?, ?)",
            (platform.node(), revision.strip(), os.cpu_count(), description, ""))
    con.commit()
    run_id = cur.execute("SELECT MAX(ROWID) FROM runs").fetchall()[0][0]

    archive = None
    if scorep:
        archive = zipfile.ZipFile('scorep_data.zip', mode='a', compression=zipfile.ZIP_LZMA)


    ms = list(range(0 if m_min == 1 else m_min, m_max + 1, m_step))
    if m_step == 1:
        ms = ms[1:]
    elif m_min == 1:
        ms[0] = 1

    for mode in modes:
        for m in ms:
            n = int(min(n_datafile, n_cutoff))
            if weak:
                n = m * int(n / max(ms))
            print(f"n={n}, m={m}")

            cluster_opts = "--bind-to core --map-by core -report-bindings" if cluster_mode else ""
            opts = f"--use-hwthread-cpus -np {m} {cluster_opts}"
            repetitions = "100"
            flags = f"-n {n}"
            cmd = f"mpirun {opts} {executable} -f {datafile} --{mode} -r {repetitions} {flags} 2>&1"
            print(f"\t{cmd}")
            env = dict(os.environ)
            scorep_dir = f"scorep_run={run_id}_n={n}_m={m}"
            env["SCOREP_EXPERIMENT_DIRECTORY"] = scorep_dir
            r = subprocess.run(cmd, env=env, shell=True, capture_output=True)
            r.check_returncode()
            output = r.stdout.decode("utf-8")
            time = grep_number("avg", output)
            stddev = grep_number("stddev", output)
            sentMessages = grep_number("sentMessages", output)
            avgSummandsPerMessage = grep_number("averageSummandsPerMessage", output)

            cur.execute('INSERT INTO results(run_id, datafile, n_summands, repetitions, mode, time_ns, stddev, output, ranks)' \
                    'VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)',
                    (run_id, datafile, n, repetitions, mode, time, stddev, output, m))
            con.commit()

            result_id = cur.execute("SELECT MAX(id) FROM results").fetchall()[0][0]
            cur.execute('INSERT INTO messages(result_id, messages_sent, avg_summands_per_message) VALUES(?, ?, ?)',
                    (result_id, sentMessages, avgSummandsPerMessage))
            con.commit()


            if scorep:
                # Write files to archive
                for f in glob.glob(scorep_dir + '/*'):
                    archive.write(f)
                shutil.rmtree("./" + scorep_dir)


    if scorep:
        archive.close()

