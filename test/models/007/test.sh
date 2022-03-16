#!/usr/bin/bash

extramem=$(( 200000 / $1 ))

mkdir -p output/spike-driven-test
mkdir -p output/needy-test

mpirun -np $1 "$2" --synch=3 --spike-driven --total-spikes=1000 --extramem=$extramem --end=100
exec mpirun -np $1 "$2" --synch=3 --total-spikes=1000 --extramem=$extramem --end=100
