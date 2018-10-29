#!/bin/bash

FREQ=800000

cpupower frequency-set -f ${FREQ}

# "fluidanimate" is a GPU application, can't measure it.
# "canneal" "x264" can't measure it.
declare -a arr=("swaptions" "facesim" "freqmine" "ferret" "blackscholes" "streamcluster" "dedup")

# source /home/tigris/parsec-3.0/env.sh

## now loop through the above array
for i in "${arr[@]}"
do
   /home/tigris/parsec-3.0/bin/parsecmgmt -a run -p ${i} -i simlarge &
   # or do whatever with individual element of the array
done

