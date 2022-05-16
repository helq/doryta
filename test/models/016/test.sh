#!/usr/bin/bash

nps=$1
doryta="$2"
modelsdir="$3"

grid_width=20

mkdir -p output

# Testing hardcoded GoL with no synaptic weight restrictions
mpirun -np 1 "$doryta" --spike-driven --output=output/gol-20x20 \
    --gol-model --gol-model-size=$grid_width \
    --load-spikes="$modelsdir"/gol/spikes/20x20/gol-die-hard.bin \
    --probe-firing --probe-firing-buffer=10000 --end=200.2 \
    --probe-stats || exit $?

# against GoL with only positive synaptic weighths
exec mpirun -np 1 "$doryta" --spike-driven --output=output/gol-20x20-non-negative \
    --load-model="$modelsdir"/gol/snn-models/gol-20x20-circuit-nonnegative.doryta.bin \
    --load-spikes="$modelsdir"/gol/spikes/20x20/gol-die-hard.bin \
    --probe-firing --probe-firing-buffer=50000 --end=199.81 \
    --probe-stats
