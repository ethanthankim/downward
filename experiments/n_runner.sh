#!/bin/bash

timestamp() {
  date '+%Y-%m-%d:%H:%M:%S' # current time
}

for i in $( seq 1 $1 )
do
    echo "Doing batch: $i -- $(timestamp)"
    ./runner.sh $2 &> "run.$i.log"

    result_dir="explore_$(timestamp)"
    mkdir "$result_dir" 

    mv src/thesis/data/thesis-explore	"$result_dir/"
    mv src/thesis/data/thesis-explore-eval "$result_dir/"
done