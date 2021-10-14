#!/usr/bin/bash

mpirun -np 2 "$2" --synch=3 --end=1 || exit $?
exec mpirun -np 2 "$2" --synch=3 --spikedriven
