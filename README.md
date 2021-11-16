# Doryta

Doryta (not an acronym) is a spiking neural network (SNN) simulator written in C using the
(massively) parallelizable [ROSS][] library for Parallel Discrete Event Simulation (PDES).

[ROSS]: https://github.com/ROSS-org/ROSS

# Compilation

Doryta requires CMake and a C compiler with support for [MPI][]. To download and compile
in as little steps, run the following instructions:

[MPI]: https://www.mpich.org/

```bash
git clone --recursive https://github.com/helq/doryta
mkdir doryta/build
cd doryta/build
cmake .. -DBUILD_TESTING=OFF
make -j4
```

In order to keep Doryta as lean as possible, all example models are stored in a separate
git submodule repo. The commands above will (potentially) take a long time to execute
(depending on your network connection) and will certainly take many unnecessary MBs in
storage. The following sequence of commands will download only the necessary data to
compile:

```bash
git clone https://github.com/helq/doryta
cd doryta
git submodule init
git submodule update --depth=1
mkdir build
cd build
cmake .. -DBUILD_TESTING=OFF
make -j4
```

After compiling, you will find the executable under the folder: `doryta/build/src`

Optionally, you can run `make install` to copy doryta to the folder where binaries are
stored in your computer (`/usr/bin` for example). If you wish to change the default
directory, for example to `doryta/build/bin` instead of `/usr/bin`, run cmake with the
flag `-DCMAKE_INSTALL_PREFIX="$(pwd -P)/"`.

# Execution

To check whether doryta has been compiled properly, run the following:

```bash
mpirun -np 2 src/doryta --sync=3 --five-example --probe-firing --probe-voltage --end=1
```

This will run a five-neuron fully connected network, on top of which a fully connected
layer of two neurons is layered. The input to the network is a fixed set of spikes, and
the parameters of the network are always the same, so that the execution of the network
must always produce the same output.

The output of running the model can be found at `output/`. The small Python script
`plot_neuron_spikes_voltage.py` under `tools/general` can be used to analyze the behaviour
of neurons and their spikes.


## MNIST example

A simple feed-forward network model for MNIST can be found under `data/models/whetstone`.
Whetstone is a library for training SNN using the Keras backend. The code to generate the
model can be found under `data/models/whetstone/code`. The testing MNIST dataset has been
spikified in order to be readable for Doryta (who only understands spikes as an input).

To inference the class of the first 20 images in the dataset, run the following command:

```bash
mpirun -np 2 src/doryta --spike-driven --synch=3 --extramem=1000000 \
    --load-model=../data/models/whetstone/simple-mnist.doryta.bin \
    --load-spikes=../data/models/whetstone/spikified-mnist/spikified-images-all.bin \
    --output-dir=output-20 \
    --probe-firing --probe-firing-output-only --probe-firing-buffer=100000 --end=19.5
```

[TO CONTINUE HERE: modify python script to use argparse, indicate in here how to run the
script and what it is doing below the surface. Continue]

## Needy vs Spike-driven modes

[Explain that the MNIST example above has been run under the spike-driven mode which
runs faster than the needy mode but by its nature doesn't store any voltage data, so it
cannot be analysed with the other python tool, that can be changed, just remove the
spike-driven flag and add the probe voltage flag with enough buffer to contain all the
execution history, mention gotchas from below]

The nature of approximating the behaviour of a neuron by simulating their behaviour one
step at the time means that there will innevitably errors, approximation errors. To
approximate better we tend to use smaller deltas. Too small of a delta and we might get
into trouble. The spike driven mode uses a different function to determine the change of
state of a neuron over a long period of time, and because it assumes a smooth transition,
not chunky, delta jumps, it will produce slightly different results on the "voltage" of
the neuron and very few spurious spikes (model dependent).

# Documentation

To generate the documentation, install [doxygen][] and dot (included in [graphviz][]), and
then run:

```bash
doxygen docs/Doxyfile
```

The documentation will be stored in `docs/html`.

[doxygen]: https://www.doxygen.nl/
[graphviz]: https://www.graphviz.org/

# Release binary

To get the clean, non-debug, faster implementation use:

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
```

# Testing

To run tests you have to compile them first and then run ctest:

```bash
cmake .. -DBUILD_TESTING=ON
make
ctest
```

Remember to run CMake again whenever a new test is added. CMake generates the rules to
compile the tests and run them. Unfortunatelly, CMake is not triggered when new
tests/folders are added to the `test` folder.
