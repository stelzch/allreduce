#!/bin/sh
host=$1

if [[ -z "$host" ]]; then
    echo "Usage: $0 <HOSTNAME>"
    exit 1
fi

workdir=allreduce-prep
echo Executing quick benchmark on $host

# copy our benchmark database to the host
scp benchmarks/results.db $host:$workdir/benchmarks/.results.db
ssh $host "cd $workdir; bash --login misc/compileandrun.sh"
scp $host:$workdir/benchmarks/results.db benchmarks/results.db

