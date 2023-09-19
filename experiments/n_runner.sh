#!/bin/bash

# for i in {1..}
# do
#     echo "Doing batch: $i"
#     # echo "y
#     # o" | ./src/thesis/explore.py --all --benchmark-dir ../benchmarks-asai
# done

for i in $( seq 1 $1 )
do
    echo "Doing batch: $i"
    echo "y
    o" | ./src/thesis/explore.py --all --benchmark-dir ../benchmarks-tiny
done