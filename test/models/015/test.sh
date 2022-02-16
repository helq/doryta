#!/usr/bin/bash

nps=$1
doryta="$2"
modelsdir="$3"

grid_width=20

# Testing GoL with random spiking inputs
exec mpirun -np $1 "$doryta" --synch=2 --spike-driven \
    --gol-model --gol-model-size=$grid_width --end=10.2 \
    --random-spikes-time=0.6 \
    --random-spikes-uplimit=$((grid_width * grid_width)) \
    --probe-stats --probe-firing --probe-firing-buffer=20000 \
    --extramem=100000
