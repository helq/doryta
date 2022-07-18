#!/usr/bin/bash

nps=$1
doryta="$2"
modelsdir="$3"

grid_width=20

mkdir -p output

# Testing GoL with static (in the middle) spike scheduling
mpirun -np $1 "$doryta" --synch=3 --spike-driven \
    --output=output/gol-middle-spikes \
    --gol-model --gol-model-size=$grid_width \
    --random-spikes-time=0.75 \
    --random-spikes-uplimit=$(( grid_width * grid_width )) \
    --probe-firing --probe-firing-buffer=10000 --end=200.001 \
    --probe-stats || exit $?

# against GoL with spikes scheduled at 
exec mpirun -np $1 "$doryta" --synch=3 --spike-driven \
    --spike-rand-sched=31 \
    --output=output/gol-random-sched-spikes \
    --gol-model --gol-model-size=$grid_width \
    --random-spikes-time=0.75 \
    --random-spikes-uplimit=$(( grid_width * grid_width )) \
    --probe-firing --probe-firing-buffer=10000 --end=200.001 \
    --probe-stats
