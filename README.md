# Doryta

Doryta (not an acronym) is a spiking neural network (SNN) simulator written in C using the
(massively) parallelizable [ROSS][] library for Parallel Discrete Event Simulation (PDES).

[ROSS]: https://github.com/ROSS-org/ROSS

# Compilation

Doryta requires CMake and a C compiler with support for [MPI][]. To download and compile
in as little steps, run the instructions below. (In order to keep Doryta as lean as
possible, all example models are stored in a separate git submodule repo. That's why the
slightly longer and complicated sequence of commands.)

[MPI]: https://www.mpich.org/

```bash
git clone https://github.com/helq/doryta
cd doryta
git submodule update --init --depth=1
patch -p1 -d external/ROSS < tools/patches/ROSS.patch
mkdir build && cd build
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

The output will be stored under the path `output-20`. The script
`tools/whetstone-mnist/check_doryta_inference.py` checks this output with the expected
output from whetstone.

## Needy vs Spike-driven modes

Doryta comes with two modes of execution: needy and spike-driven. In _Needy_ mode, Doryta
will update the state of each neuron every delta time just as any other ordinary
simulation framework. In _spike-driven_ mode, Doryta will only update the state of the
neuron when it receives a spike, and it's, thus, much faster than _needy_. The main
assumption for _spike-driven_ to behave as _needy_ is that there must NEVER be positive
leak on the system, ie, neurons cannot spike on their own if there don't have a input
stimulous (spike).

The MNIST example (above) uses the _spike-driven_ mode, which runs faster than the _needy_
mode, but it doesn't compute the step by step change on voltage. This means that we cannot
analyze the voltage behaviour of neurons. If you want to analyze the voltage change,
remove the `--spike-driven` flag, activate the voltage probe (`--probe-voltage`) and
assign enough buffer space (`--probe-voltage-buffer`) to store voltage for all neurons on
the determined time step.

_Note on custom models_: There might be some discrepancies when running a model on the
spike-driven mode opposed to needy mode. To reduce such discrepancies, we recommend to
make the heartbeat interval (the delta of the approximation) small enough. By the very
nature of simulation, breaking a continuous equation into discrete steps, there will
always be a tradeoff between computation time and fidelity.

# Release binary

To get the clean, non-debug, faster implementation use:

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
```

# Testing

To run tests you have to compile them first and then run ctest:

```bash
cmake .. -DBUILD_TESTING=ON
make -j4
ctest -j4
```

Remember to run CMake again whenever a new test is added. CMake generates the rules to
compile the tests and run them. Unfortunatelly, CMake is not triggered when new
tests/folders are added to the `test` folder.
