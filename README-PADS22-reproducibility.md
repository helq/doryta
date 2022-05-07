# Reproducing results from "Evaluating Performance of Spintronics-Based Spiking Neural Network Chips using Parallel Discrete Event Simulation"

This folder contains all the data and instructions to reproduce the results and figures
shown in the paper. Some steps are optional, and when run they will produce different,

This document is broken into four sections: software requirements, how to compile doryta,
steps to reproduce energy estimation results and steps to reproduce strong scaling
results.

## A. Software requirements

Doryta is written in C. You need a C compiler, CMake and support for [MPI][].

All surrounding scripts for the analysis of the data are written in Python 3. All Python
code has been tested in Python 3.9 and 3.10. Additionally, the following Python libraries
are needed for the scripts to work:

- Tensorflow 2.8 (for ANN model training, saving and checking)
- Keras 2.8 (for ANN model training, saving and checking)
- Numpy 1.22.3
- Matplotlib 3.5.1

Other versions of the libraries might work, but these are the versions we have tested them
against.

## B. Doryta: Compilation

Doryta requires CMake 3.21 and a C-17 compiler with support for [MPI][]. (In fact, C-11 is
sufficient to compile Doryta (C-11 is a superset of C-17) and older versions of CMake
might also work.)

[MPI]: https://www.mpich.org/

To compile run:

```bash
mkdir build && cd build
cmake .. -DBUILD_TESTING=OFF
make
```

After compiling, you will find Doryta's executable under the folder: `build/src`

Optionally, you can run `make install` to copy Doryta to the folder where binaries are
stored in your computer (`/usr/bin` for example). If you wish to change the default
directory, for example to `doryta/build/bin` instead of `/usr/bin`, run cmake with the
flag `-DCMAKE_INSTALL_PREFIX="$(pwd -P)/"`.

*Note:* In some cases, it has been reported that Doryta can't be compiled due to
a missing include. If this is your case, please uncomment lines 13 and 14 in
`CMakeList.txt` and try again. :)

## C. Reproducing energy estimation results

Assuming that the directory where this readme is situated is `<doryta-root>/`, you can
reproduce the results found in the paper in two ways: run the script `reproduce_pads22.sh`
to reproduce the entire procedure; or follow the step by step instructions below. We
suggest you check the step-by-step procedure as scripts tend to break as software
progresses :(.

### The everything-bundled script

Running this script will execute ALL steps from below. This means that it will take a
significant amount of time, probably many hours, and its result will be slightly
different, but hopefully similar enough, to the results presented in the paper.

```bash
cd "<doryta-root>/"
bash -x data/experiments/performance_estimation/reproduce_pads22.sh
# or, to store all logs
bash -x data/experiments/performance_estimation/reproduce_pads22.sh > logs.txt
```

After running the whole script satisfactorily, there will be three new files under
`<doryta-root>/data/experiments/performance_estimation`: `combine.eps` (Figure 7),
`combine.csv` (Table 7) and `table6.csv` (Table 6).

### Step-by-step:

Anything marked optional can be skipped, in fact, everything but the last step can be
skipped if you only want to regenerate Figure 7 and Table 7:

1. (optional) Train the SNN model using Keras and Whetstone. This step defines, trains and
    saves in a format that Doryta can understand the four network architectures presented
    in the paper (lenet small, lenet small fashion, lenet large, and lenet large fashion).

    ```bash
    cd <doryta-root>/data/models
    python -m code.lenet_mnist --train --save
    python -m code.lenet_mnist --train --save --fashion
    python -m code.lenet_mnist --train --save --large-lenet
    python -m code.lenet_mnist --train --save --fashion --large-lenet
    ```

    After running a single command a SNN trained in MNIST (or Fashion-MNIST)
    should be generated as a `.doryta.bin` file under
    `<doryta-root>/data/models/mnist/ssn-models`. This file is readable by Doryta only.

    If you don't want to train the model and you simply want to see the accuracy results
    run `python lenet_mnist.py`.

    *Note: training a new model will produce slightly different results to those presented
    in the paper. This step should be skipped if you want to __replicate__ the results in
    the paper.*

2. (optional) Generate input spikes for each model. This takes the MNIST dataset, or
    MNIST-Fashion dataset, and generates a binary file with the images encoded as spikes,
    which can be loaded into Doryta.

    ```bash
    cd <doryta-root>/data/models
    python -m code.ffsnn_mnist
    python -m code.ffsnn_mnist --fashion
    ```

    This will create four files under `data/models/mnist/spikes/`, two per each
    invocation. Files with the extension `.bin` contain the spike information; files with
    the extension `.tags.bin` contain the tags given to each element.

3. Run the SNN model in Doryta and tell Doryta to store the stats. These stats contain the
    amount of leak, integration and firing operations performed by each neuron.

    ```bash
    cd <doryta-root>/build/
    mpirun -np 1 src/doryta --spike-driven --synch=3 \
        --load-model=<doryta-root>/data/models/mnist/snn-models/lenet-mnist-filters=6,16.doryta.bin \
        --load-spikes=<doryta-root>/data/models/mnist/spikes/spikified-mnist/spikified-images-all.bin \
        --output-dir='mnist-lenet-small' \
        --probe-firing --probe-firing-buffer=1000000 --probe-stats --probe-firing-output-only \
        --extramem=10000000 --end=10000
    ```

    If Doryta fails to run and the reason is `--extramem`, increase the size of
    extramemory and try again.

    Notice that running this model will take at least 12 GiB of RAM (mostly occupied to
    load the initial spike information). The simulation might take an hour or longer to
    run in sequential mode. To speed it up, replace `-np 1` for `-np X`, where `X` is any
    natural number and equal to the number of physical cores available in your computer.
    `--extramem` and `--probe-firing-buffer` are inversely correlated to the number of
    cores/ranks used for the simutlation. For `-np 2`, divide `--extramem` and
    `--probe-firing-buffer` by two to save space.

    If you are not verifying the output from Doryta with that of Keras, you can run the
    command without the option `--probe-firing`.

    Once the simulation finishes, the folder `<doryta-root>/build/mnist-lenet-small`
    should contain all at least three plain text files. This command must be run once for
    every model defined (four in total: Small LeNet MNIST, Large LeNet MNIST, Small LeNet
    Fashion-MNIST and Large LeNet Fashion-MNIST).

4. (optional) Check that Doryta's output corresponds 1-to-1 to that of the Keras model
    that it is based on:

    ```bash
    # preparing code
    ln -fs <doryta-root>/data/models/code <doryta-root>/tools/whetstone-mnist/ws_models

    # checking
    python <doryta-root>/tools/whetstone-mnist/check_doryta_inference.py \
        --path-to-keras-model <doryta-root>/data/models/mnist/raw_keras_models/lenet-mnist-filters=6,16 \
        --path-to-tags <doryta-root>/data/models/mnist/spikes/spikified-mnist/spikified-images-all.tags.bin \
        --indices-in-output 10000 \
        --outdir-doryta <doryta-root>/build/mnist-lenet-small \
        --dataset mnist --model-type lenet
    ```

    The result of running this script should be a positive message on the screen informing
    you that the output from Keras and Doryta are identical. When checking the large LeNet
    models an additional parameter must be used `--shift 38348`

5. Create statistics files from the data. Each file contains the workloads used by the
    benchmarking procedure. Neurons are grouped into their corresponding crossbars.

    ```bash
    cd <doryta-root>/build/
    python <doryta-root>/tools/general/total_stats.py \
        --path mnist-lenet-small \
        --iterations 10000 --csv <doryta-root>/data/experiments/performance_estimation/small_lenet \
        --groups '[784,784,784,784,784,784,784,196,196,196,196,196,196,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,120,84]'
    ```

    The `--groups` parameter for the large LeNet architecture is:

    ```
    '[784,784,784,784,784,784,784,784,784,784,784,784,784,784,784,784,784,784,784,784,784,784,784,784,784,784,784,784,784,784,784,784,784,196,196,196,196,196,196,196,196,196,196,196,196,196,196,196,196,196,196,196,196,196,196,196,196,196,196,196,196,196,196,196,196,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,120,84]'
    ```

    (yeah, it gets a bit out of hand when the network architecture is laaarge :S)

6. The final script takes the statistics file in `csv` from all four models (small LeNet
    MNIST, sl; small LeNet Fashion-MNIST, slf; large LeNet MNIST, ll; large LeNet
    Fashion-MNIST, llf). This is the benchmarking script. It computes the energy, latency,
    area and other parameters presented in the paper:

    ```bash
    dir_to_data="<doryta-root>/data/experiments/performance_estimation"
    python "<doryta-root>/external/tools/performance spintronics estimation"/benchmarking.py \
        --path-ll "$dir_to_data"/large_lenet-average.csv \
        --path-sl "$dir_to_data"/small_lenet-average.csv \
        --path-llf "$dir_to_data"/large_lenet_fashion-average.csv \
        --path-slf "$dir_to_data"/small_lenet_fashion-average.csv \
        --output "$dir_to_data"/combine
    ```

    The output of running the script is the figure `combine.eps` (Figure 7 in the paper),
    and `combine.csv` (Table 7 in paper). Both are under the folder
    `data/experiments/performance_estimation`.

    The content of Table 1 is hardcoded in the script `benchmarking.py` in the
    variables named: `bey_Mn3Ir` and `bey_NiO`

## D. Reproducing strong scaling results

Included in the folder `<doryta-root>/data/experiments/gol/scaling/` are three scripts used
to produce the strong scaling results on conservative, optimistic and optimistic realtime
modes. The scripts assume that Doryta is running on a AiMOS node, thus some changes to the
scripts are most certainly required to run on other systems.

Given the diversity of systems, it is impossible to present a single general script, or a
collection of them, to reproduce the scaling results in anything that is not current AiMOS
(yes, the scripts might fail even in AiMOS in the future).

Yet, if you want to reproduce the scaling results and you have a system where to do it,
the following are the basic instructions we've used for the three scaling cases. Make sure
to run as in many MPI ranks/physical cores available. Hyperthreading does not show
significant improvements. The results of running Doryta can be found in the `ross.csv`
file. It is from this document that Table 4 and 5 were produced.

### Conservative mode

You can replace `np=2` for any number of cores you have accessible.

```bash
grid_width=1000
np=2
outdir=gol-conservative/$grid_width-np=$np
mkdir -p $outdir

mpirun -np $np "<doryta-root>/build/src/doryta" --synch=2 --spike-driven \
        --gol-model --gol-model-size=$grid_width --end=1000.2 \
        --random-spikes-time=0.6 \
        --random-spikes-uplimit=$((grid_width * grid_width)) \
        --output-dir=$outdir --extramem=$((40000000 / np)) > $outdir/model-result.txt
```

### Optimistic mode

```bash
grid_width=1000
np=2
outdir=gol-optimistic/$grid_width-np=$np
mkdir -p $outdir

mpirun -np $np "<doryta-root>/build/src/doryta" --synch=3 --spike-driven \
        --max-opt-lookahead=1 \
        --gol-model --gol-model-size=$grid_width --end=1000.2 \
        --random-spikes-time=0.6 \
        --random-spikes-uplimit=$((grid_width * grid_width)) \
        --output-dir=$outdir --extramem=$((40000000 / np)) > $outdir/model-result.txt
```

### Optimistic realtime mode

```bash
grid_width=1000
np=2
outdir=gol-optimistic-realtime/$grid_width-np=$np
mkdir -p $outdir

mpirun -np $np "<doryta-root>/build/src/doryta" --synch=5 --spike-driven \
        --max-opt-lookahead=1 \
        --gvt-interval=1 --batch=4 \
        --gol-model --gol-model-size=$grid_width --end=1000.2 \
        --random-spikes-time=0.6 \
        --random-spikes-uplimit=$((grid_width * grid_width)) \
        --output-dir=$outdir --extramem=$((40000000 / np)) > $outdir/model-result.txt
```
