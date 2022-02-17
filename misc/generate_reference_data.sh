#!/bin/sh
for file in data/*; do
    ./build/src/RADTree \
        -f "$file" --tree \
        | grep sumBytes \
        | awk -F'=' "{ print \"$file\", \$2 }"
done
