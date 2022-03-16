#!/usr/bin/bash

mkdir -p output/spike-driven-test
mkdir -p output/needy-test

mpirun -np 2 "$2" --synch=3 --end=1 || exit $?
exec mpirun -np 2 "$2" --synch=3 --spikedriven
