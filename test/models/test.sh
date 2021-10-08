#!/usr/bin/bash

# $1 = is binary
# $2 = number of mpi ranks
# $3 = simulation limit

if [ -f "ross.csv" ]; then
  rm ross.csv
fi

exec mpirun -np "$2" "$1" --end="$3" --gvt-interval=32 --extramem=16384 --synch=3 --batch=2
