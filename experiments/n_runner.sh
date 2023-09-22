#!/bin/bash

timestamp() {
  date +"%T" # current time
}

for i in $( seq 1 $1 )
do
    echo "Doing batch: $i -- $(date +%T)"
    ./runner.sh $2 &> "run.$i.log"

    result_dir="explore_$(date --date=@${DATE} '+%Y-%m-%d:%H:%M:%S')"
    mkdir "$result_dir" 

    mv src/thesis/data/thesis-explore	result_dir/
    mv src/thesis/data/thesis-explore-eval result_dir/
done