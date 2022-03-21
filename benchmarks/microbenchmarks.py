import io
import csv
import sqlite3
import argparse
import subprocess
import platform
from benchmark import init_db

if __name__ == '__main__':
    repetitions = 20

    git_result = subprocess.run(["git", "rev-parse", "HEAD"], capture_output=True)
    revision = str(git_result.stdout, 'utf-8') if git_result.returncode == 0 else "NONE"

    uname_result = subprocess.run(["uname", "-a"], capture_output=True, check=True)
    uname = str(uname_result.stdout, 'utf-8')

    hostname = platform.node()

    r = subprocess.run(f"./build/benchmarks/benchmark --benchmark_format=csv --benchmark_repetitions={repetitions}", shell=True, capture_output=True, check=True)
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

    results = dict()
    for name,iterations,real_time,cpu_time,time_unit,_,_,_,_,_ in reader:
        if name.endswith('_mean') or name.endswith('_stddev'):
            name = name.replace("/iterations:1", "")
            results[name] = real_time
    for mean_key in filter(lambda x: x.endswith('_mean'), results.keys()):
        key = mean_key.replace('_mean', '')

        mean_val = float(results[mean_key])
        stddev_val = float(results[key + '_stddev'])

        cur.execute('INSERT INTO microbenchmark_results(run_id, benchmark, time, stddev, repetitions) VALUES (?, ?, ?, ?, ?)',
                (run_id, key, mean_val, stddev_val, repetitions))
    con.commit()
