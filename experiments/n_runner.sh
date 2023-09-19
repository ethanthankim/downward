#!/bin/bash

timestamp() {
  date +"%T" # current time
}

for i in $( seq 1 $1 )
do
    echo "Doing batch: $i -- $(date +%T)"
    ./runner.sh $2 &> "run.$i.log"
done