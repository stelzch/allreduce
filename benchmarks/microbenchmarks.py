import io
import csv
import sqlite3
import argparse
import subprocess
import platform
from benchmark import init_db

if __name__ == '__main__':

    git_result = subprocess.run(["git", "rev-parse", "HEAD"], capture_output=True)
    revision = str(git_result.stdout, 'utf-8') if git_result.returncode == 0 else "NONE"

    uname_result = subprocess.run(["uname", "-a"], capture_output=True, check=True)
    uname = str(uname_result.stdout, 'utf-8')

    hostname = platform.node()

    r = subprocess.run("./build/benchmarks/benchmark --benchmark_format=csv", shell=True, capture_output=True, check=True)
    output = str(r.stdout, 'utf-8')

    con = sqlite3.connect('file:benchmarks/results.db?ro=true', uri=True)
    cur = con.cursor()
    init_db(cur)

    cur.execute('INSERT INTO microbenchmark_runs(hostname,revision,uname,date,repetitions) VALUES(?,?,?,datetime(),1)',
            (hostname, revision.strip(), uname.strip()))

    cur.execute('SELECT MAX(id) FROM microbenchmark_runs')
    run_id = cur.fetchone()[0]

    reader = csv.reader(io.StringIO(output))
    next(reader) # skip header
    for name,iterations,real_time,cpu_time,time_unit,_,_,_,_,_ in reader:
       cur.execute('INSERT INTO microbenchmark_results(run_id, benchmark, time) VALUES (?, ?, ?)',
               (run_id, name, real_time))
    con.commit()
