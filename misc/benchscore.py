#!/usr/bin/env python3
import sqlite3
from scipy.stats import linregress

con = sqlite3.connect("file:benchmarks/results.db?mode=ro", uri=True)

cur = con.cursor()
cur.execute('SELECT * FROM runs')

run_headers = list(map(lambda x: x[0], cur.description))
runs = cur.fetchall()

def get_timings(mode, run_id):
    cur.execute("SELECT n_summands, time_ns / 1e9 FROM results WHERE mode = ? AND run_id = ?",
            (mode, run_id))
    r = cur.fetchall()
    return list(map(lambda x: x[0], r)), list(map(lambda x: x[1], r))


#print("slowdown", "id", "date", "hostname", "revision", "cluster_size", "description", "flags", sep=",")

for run in runs:
    run_id = run[0]

    # get Allreduce results
    x1, allreduce_timings = get_timings("allreduce", run_id)
    x2, tree_timings = get_timings("tree", run_id)

    g1, _, r1, _, _ = linregress(x1, allreduce_timings)
    g2, _, r2, _, _ = linregress(x2, tree_timings)

    slowdown = g2 / g1

    
    print(slowdown, *run, sep=",")
