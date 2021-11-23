#!/bin/sh
python3 misc/benchscore.py | \
    column --table --separator , \
    --table-columns "slowdown,id,date,hostname,revision,cluster_size,description,flags" \
    --table-truncate description,revision \
    --output-width 80 | less -S
