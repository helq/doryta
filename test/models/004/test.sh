#!/usr/bin/bash

if [ -f "ross.csv" ]; then
  rm ross.csv
fi

exec mpirun -np 2 "$1" --gvt-interval=32 --extramem=16384 --synch=3 --end=1 --batch=2
