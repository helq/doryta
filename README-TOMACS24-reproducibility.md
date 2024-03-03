# Reproducing results from article in TOMACS journal (doi:10.1145/3649464)

This folder contains all the data and instructions to reproduce some results and figures
shown in the paper: [Performance Evaluation of Spintronic-Based Spiking Neural Networks Using
Parallel Discrete-Event Simulation][doi-paper]. Some steps are optional, and when run they
will produce slightly different results from those of the paper.

[doi-paper]: https://doi.org/10.1145/3649464

This document is broken into four sections: software requirements, how to compile doryta,
steps to reproduce energy estimation results and steps to reproduce strong scaling
results.

## A. Software requirements

Doryta is written in C. You need a C compiler, CMake and support for [MPI][].

All surrounding scripts for the analysis of the data are written in Python 3. All Python
code has been tested in Python 3.10. Additionally, the following Python libraries are
needed for the scripts to work (either version works):

- Tensorflow 2.8 (for ANN model training, saving and checking)
- Keras 2.8 (for ANN model training, saving and checking)
- Numpy 1.22.3
- Matplotlib 3.5.1
- Jupyter Notebook 6.5.2

Other versions of the libraries might work, but these are the versions we have tested them
against.

For ease of installation, you can install all dependencies with:

```bash
pip install -r requirements.txt
```

The line above has been tested with Python 3.10 on a x64 intel machine with no GPU.

**Note:** Running any version other than those suggested in here might break the scripts
below in an unpredictable way. The code is offered as-is, although we assure you that at
the time of writing it works as intended on the systems that have been tested.

## B. Doryta: Compilation

Doryta requires CMake 3.21 and a C-17 compiler with support for [MPI][]. (In fact, C-11 is
sufficient to compile Doryta (C-11 is a superset of C-17) and older versions of CMake
might also work.)

[MPI]: https://www.mpich.org/

To compile:

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
a missing include in the `CMakeList.txt` file. If this is your case, please uncomment
lines 13 and 14 in `CMakeList.txt` and try again. :)

*Note 2:* You might encounter a CMake warning, especifically CMP0118. Do not worry about
this warning, it comes from ROSS but it doesn't alter/break the compilation or binaries
(as of now).

## C. Reproducing performance estimation results

You can reproduce the results found in the paper in two ways: run the script
`reproduce_tomacs23.sh` (see below) to reproduce the entire procedure; or, follow the step
by step instructions (see below below). We suggest you check the step-by-step procedure as
automated scripts tend to break with time as new software versions replace those in which
the code was originally written :(. After that, you have to run the `Benchmark.ipynb`
jupyter notebook to obtain two figures and a table.

**Note:** we assume that the directory where this readme is situated is `<doryta-root>/`.

### The (almost) everything-bundled script

Running this script will execute all steps from the next subsubsection (the step-by-step)
except for step 7, which requires running a jupyter notebook. That has to be run manually.
This means that it might take a significant large amount of time, probably many hours,
and the final results will be slightly different, but hopefully similar enough, to the
results presented in the article.

```bash
cd "<doryta-root>/"
bash -x data/experiments/performance_estimation/reproduce_tomacs23.sh
# or, to store all logs
bash -x data/experiments/performance_estimation/reproduce_tomacs23.sh > logs.stdout.txt 2> logs.stderr.txt
```

After running the whole script satisfactorily, there will be one new file under
`<doryta-root>/data/experiments/performance_estimation`: `table11.csv` (Table 11).

### Step-by-step:

Anything marked optional can be skipped, in fact, everything but the last step can be
skipped if you only want to regenerate Figures 9 and 10, and Table 11:

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

    Running each command/line above will train a SNN in MNIST (or Fashion-MNIST) and it
    will save the model under `<doryta-root>/data/models/mnist/ssn-models` with the file
    extension `.doryta.bin`. This file is readable by Doryta only.

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
    rm -rf "$dorytaroot"/tools/whetstone-mnist/ws_models/code
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

    Notice that Doryta and the Keras model correspond 1-to-1 for the architectures we have
    checked. We trained and tested the Keras model with no optimizations on CPU-only. When
    running the inference the model in GPU, the results between keras and doryta might
    differ. This is the nature of running different architectures where floating point
    operations don't match 1-to-1. Optimized ML models are often not guaranteed to
    correspond 1-to-1 to their unoptimized counterparts. Recent versions of Keras heavily
    optimize code and seem to break the correspondence we have seen historically seen.

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

6. Generate Table 11. Run:

    ```bash
    cd <doryta-root>/data/experiments/performance_estimation

    truncate -s 0 table11.csv # deleting/creating file
    echo "workload,integration,fire,accuracy" >> table11.csv
    for f in {small_lenet,small_lenet_fashion,large_lenet,large_lenet_fashion}; do
        # saving workload name
        printf "$f," >> table11.csv
        # saving integration and fire stats
        awk -F, 'NR == 2 { printf "%s,%f,", $4, $5 }' $f-average.csv >> table11.csv
        # saving accuracy stats
        cat ${f}_accuracy.txt >> table11.csv
    done

    ```

    The table will be saved into the doc `table11.csv`.

7. Running the jupyter notebook script

   Launch jupyter notebook and open the notebook
   `<doryta-root>/external/tools/performance spintronics estimation/v2/Benchmarking.ipynb`.

   The result of running all cells will produce Figures 7 and 8, and Table 4. The figures
   will be stored in the folder `<doryta-root>/figures`.

## D. Reproducing strong scaling results

Included in the folder `<doryta-root>/data/experiments/gol/scaling/v2` are all the scripts
we used to run the scaling-up experiments presented in the paper. The scripts assume that
Doryta is running on AiMOS (the supercomputer running at CCI (Center for Computational
Innovations)), thus some changes to the scripts are most certainly required to run on
other systems.

Given the diversity of systems, it is impossible to present a single general script to
reproduce the scaling results in anything that is not current AiMOS (yes, we have had
scripts failing to work after upgrades on the supercomputer).

Yet, if you want to reproduce the scaling results and you have a system where to do it,
the following are the basic instructions we've used. Make sure to run as in many MPI
ranks/physical cores available. Hyperthreading does not show significant improvements. The
raw results of the experiments can be found in the folder
`<doryta-root>/data/experiments/gol/scaling/v2/results`.

The following are the scripts' names and results for each of the tables:

- Tables 5 and 7.
    Scripts:
    * `nodes=1.sh`
    * all scripts under `scale-up-nodes/`

    Results stored in:
    * `results/conservative-vs-optimistic-gridsize=1024.ods`, and
    * `results/conservative-vs-optimistic-gridsizes=1K,2K,4K,8K.ods`

- Tables 6 and 8.
    Scripts:
    * `no-tiebreaker/nodes=1.sh`
    * all scripts under `no-tiebreaker/scale-up-nodes/`

    Results stored in:
    * `results/no-tiebreaker/conservative-vs-optimistic-gridsize=1024.ods` and
    * `results/no-tiebreaker/conservative-vs-optimistic-gridsizes=1K,2K,4K,8K.ods`, respectively.

- Table 10.
    Script: `intel-cache-experiment.sh`

    Model output: `results/doryta-gol-1024-x86-results-Chris.zip`

- Figure 9.
    Data taken out of
    * `results/conservative-vs-optimistic-gridsizes=1K,2K,4K,8K.ods` and
    * `results/no-tiebreaker/conservative-vs-optimistic-gridsizes=1K,2K,4K,8K.ods`

    into
    * `results/weak-scaling-1K-2K-4K-8K.ods`
    * `results/no-tiebreaker/weak-scaling-1K-2K-4K-8K.ods`

- Table 9.
    Data taken out of
    * `results/conservative-vs-optimistic-gridsizes=1K,2K,4K,8K.ods` and
    * `results/no-tiebreaker/conservative-vs-optimistic-gridsizes=1K,2K,4K,8K.ods`

    into `conservative-vs-optimistic-with-and-wo-tiebreaker.ods`

- Figure 10.
    Script: `smudging-conservative-nodes=1.sh`

    Results into: `results/smudging-inherent-lookahead.ods`

### Conservative mode

You can replace `np=2` for any number of cores you have accessible. To test different grid
sizes, modify `grid_size`. To run experiments with the tie-breaker deactived, you have to
recompile Doryta with `USE_RAND_TIEBREAKER` set to OFF. Simply run
`cmake -DUSE_RAND_TIEBREAKER=OFF ..` followed by `make`.

```bash
grid_width=1024
np=2
outdir=gol-conservative/$grid_width-np=$np
mkdir -p $outdir

mpirun -np $np "<doryta-root>/build/src/doryta" --synch=2 --spike-driven \
        --cons-lookahead=4.0 \
        --gvt-interval=128 --batch=512 \
        --gol-model --gol-model-size=$grid_width \
        --heartbeat=20 --end=40000.2 \
        --random-spikes-time=5.0 \
        --random-spikes-uplimit=$((grid_width * grid_width)) \
        --output-dir=$outdir --extramem=$((40000 * grid_width / np)) > $outdir/model-result.txt
```

### Optimistic mode

```bash
grid_width=1024
np=2
outdir=gol-optimistic/$grid_width-np=$np
mkdir -p $outdir

mpirun -np $np "<doryta-root>/build/src/doryta" --synch=3 --spike-driven \
        --max-opt-lookahead=10 \
        --gvt-interval=1000000 --nkp=128 --batch=32 \
        --gol-model --gol-model-size=$grid_width \
        --heartbeat=20 --end=40000.2 \
        --random-spikes-time=5.0 \
        --random-spikes-uplimit=$((grid_width * grid_width)) \
        --output-dir=$outdir --extramem=$((10 * grid_width * grid_width / np)) > $outdir/model-result.txt
```
