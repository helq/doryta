# Doryta

Doryta (not an acronym) is a spiking neural network simulator written in C using the
(massively) parallelizable [ROSS][] library for Parallel Discrete Event Simulation (PDES).

[ROSS]: https://github.com/ROSS-org/ROSS

# Compilation

The following are the instructions to download and compile:

```bash
git clone --recursive https://github.com/helq/doryta
mkdir doryta/build
cd doryta/build
cmake .. -DBUILD_TESTING=OFF
make
```

After compiling, you will find the executable under the folder: `build/src`

Optionally, you can run `make install` to copy doryta to the folder where binaries are
stored in your computer. One place for easy access while developing doryta is `build/bin`.
For this run cmake with the flag `-DCMAKE_INSTALL_PREFIX="$(pwd -P)/"`.

# Execution

Asuming you have downloaded the example models with
`git clone --depth=1 https://github.com/helq/doryta-models data/models`, you can simply
run doryta with:

...

An example of running in one or two cores:

```bash
cd build
bin/doryta --help
mpirun -np 2 bin/doryta --sync=3 --end=1
```

Inside the directory `output/` there will be output files from the simulation.

Running a complete inference of doryta on MNIST dataset:

```bash
mpirun -np 2 src/doryta --spike-driven --load-model=../data/models/whetstone/simple-mnist.doryta.bin --synch=3 --load-spikes=../data/models/whetstone/spikified-mnist/spikified-images-all.bin --extramem=1000000 --end=10001 --probe-firing --probe-firing-output-only --probe-firing-buffer=100000 --output-dir=output-all
```

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
