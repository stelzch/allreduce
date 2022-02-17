import sys
import re
import argparse
import sqlite3
import csv
import binascii
import struct

def hex2float(s):
    value, = struct.unpack('>d', binascii.unhexlify(s))
    return value

def load_reference_values(path='misc/reference_tree_sum.csv'):
    reference_values = {}
    with open(path, 'r') as f:
        r = csv.reader(f)

        for dataset, reference_sum in r:
            reference_values[dataset] = reference_sum

    return reference_values

def retrieve_calculated_sums(cur, run_id, mode = 'tree'):
    cur.execute('SELECT id,datafile,output FROM results WHERE run_id = ? AND mode = ?', (run_id, mode))

    for row in cur.fetchall():
        result_id, dataset, output = row
        actualSum = None
        actualSumBytes = None
        m = re.search("^sum=(.+)$", output, flags=re.MULTILINE)
        if m is not None:
            actualSum = float(m.group(1))
            
        m = re.search('^sumBytes=([0-9A-F]+)$', output, flags=re.MULTILINE)
        if m is not None:
            actualSumBytes = m.group(1)

        yield(result_id, dataset, actualSum, actualSumBytes)
        

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Check database benchmark runs against reference values')
    parser.add_argument('run_id', type=int, help='Run ID to check for errors')

    args = parser.parse_args()

    run_id = args.run_id

    reference_values = load_reference_values()
    con = sqlite3.connect('file:benchmarks/results.db?mode=ro', uri=True)
    cur = con.cursor()

    checked = 0
    for result_id, dataset, actualSum, sumBytes in retrieve_calculated_sums(cur, run_id):
        checked += 1
        if None in (actualSum, ):
            continue

        if dataset not in reference_values.keys():
            print(f"Missing reference entry for dataset '{dataset}'", file=sys.stderr)
            continue

        ref = hex2float(reference_values[dataset])

        if ref != actualSum:
            print(f"Mismatch for result id {result_id}, dataset {dataset}, expected {ref} but got {actualSum}",
                    file=sys.stderr)
        if sumBytes is not None and sumBytes != reference_values[dataset]:
            print(f"Bytewise mismatch for result id {result_id}, dataset {dataset}, expected {reference_values[dataset]} but got {sumBytes}")
    print(f"[INFO] Checked {checked} results")


