#!/usr/bin/bash

nps=$1
doryta="$2"
modelsdir="$3"

# Testing LeNet
exec mpirun -np $1 "$doryta" --synch=3 --spike-driven \
    --load-model="$modelsdir"/mnist/snn-models/lenet-mnist-filters=6,16.doryta.bin \
    --load-spikes="$modelsdir"/mnist/spikes/spikified-mnist/spikified-images-20.bin \
    --probe-stats --probe-firing --probe-firing-buffer=20000 --extramem=100000
