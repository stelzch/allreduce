import shutil
import os
import sqlite3
import argparse
import sys
import platform
import zipfile
import subprocess
import glob
from pycubexr import CubexParser
from benchmark import grep_number, init_db

def get_n(datafile):
    if datafile.endswith('binpsllh'):
        return (os.path.getsize(datafile) - 8) / 8 # -8 for 64-bit header, 8 bytes per double
    else:
        # text format, simply count lines
        with open(datafile, 'r') as f:
            n_summands = len(f.readlines()) - 1

        return n_summands

# Return [number of messages, number of awaited messages, bytes_sent] or None
def get_message_stats(cubefile):

    if not os.path.exists(cubefile):
        return None

    with CubexParser(cubefile) as p:
        visits = p.get_metric_values(p.get_metric_by_name("visits"))
        bytes_sent = p.get_metric_values(p.get_metric_by_name("bytes_sent"))

        r_isend = p.get_region_by_name("MPI_Isend")
        r_wait = p.get_region_by_name("MPI_Wait")

        def total(region, metric):
            return sum([sum(metric.cnode_values(cnode)) for cnode in p.get_cnodes_for_region(region.id)])

        return [total(r_isend, visits), total(r_wait, visits), total(r_isend, bytes_sent)]


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

    for m in ms:
        n = int(min(n_datafile, n_cutoff))
        if weak:
            n = m * int(n / max(ms))
        print(f"n={n}, m={m}")

        opts = f"--use-hwthread-cpus -np {m}"
        mode = "--tree"
        repetitions = "100"
        flags = f"-n {n}"
        cmd = f"mpirun {opts} {executable} -f {datafile} {mode} -r {repetitions} {flags} 2>&1"
        print(f"\t{cmd}")
        env = dict(os.environ)
        scorep_dir = f"scorep_run={run_id}_n={n}_m={m}"
        env["SCOREP_EXPERIMENT_DIRECTORY"] = scorep_dir
        r = subprocess.run(cmd, env=env, shell=True, capture_output=True)
        r.check_returncode()
        output = r.stdout.decode("utf-8")
        time = grep_number("avg", output)
        stddev = grep_number("stddev", output)

        cur.execute('INSERT INTO results(run_id, datafile, n_summands, repetitions, mode, time_ns, stddev, output, ranks)' \
                'VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)',
                (run_id, datafile, n, repetitions, mode[2:], time, stddev, output, m))
        con.commit()
        if scorep:
            # Write files to archive
            for f in glob.glob(scorep_dir + '/*'):
                archive.write(f)
            # Write message count in db
            r = get_message_stats(scorep_dir + '/profile.cubex')
            if r == None:
                print("[ERROR] Could not read cubex file")
                continue
            result_id = cur.execute("SELECT MAX(id) FROM results").fetchall()[0][0]
            cur.execute('INSERT INTO messages(result_id, messages_sent, messages_awaited, bytes_sent) VALUES(?, ?, ?, ?)',
                    (result_id, *r))
            con.commit()
            shutil.rmtree("./" + scorep_dir)


    if scorep:
        archive.close()

