#!/bin/bash

for i in $( seq 1 $1 )
do
    echo "Doing batch: $i"
    ./runner.sh $2
done