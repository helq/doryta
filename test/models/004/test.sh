#!/usr/bin/bash

exec mpirun -np $1 "$2" --gvt-interval=32 --extramem=16384 --synch=3 --end=1 --batch=2
