#!/usr/bin/bash

nps=$1
doryta="$2"
modelsdir="$3"

# Testing GoL
exec mpirun -np $1 "$doryta" --synch=3 --spike-driven --gol-model \
    --load-spikes="$modelsdir"/gol-spikes/20x20/gol-glider.bin \
    --end=80 --probe-stats --probe-firing
