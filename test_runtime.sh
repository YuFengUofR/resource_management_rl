#!/bin/bach

FREQ=800000

cpupower frequency-set -f ${FREQ}

declare -a arr=("swaptions" "facesim" "freqmine" "ferret" "fluidanimate" "blackscholes" "streamcluster" "dedup" "canneal")

# source /home/tigris/parsec-3.0/env.sh

## now loop through the above array
for i in "${arr[@]}"
do
   /home/tigris/parsec-3.0/bin/parsecmgmt -a run -p ${i} -i simlarge
   # or do whatever with individual element of the array
done

