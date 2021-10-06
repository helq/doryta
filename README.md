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

An example of running in one or two cores:

```bash
cd build
bin/doryta --help
mpirun -np 2 bin/doryta --sync=3 --end=1
```

Inside the directory `output/` there will be output files from the simulation.

# Documentation

To generate the documentation, install [doxygen][] and dot (included in [graphviz][]), and
then run:

```bash
doxygen docs/Doxyfile
```

The documentation will be stored in `docs/html`.

[doxygen]: https://www.doxygen.nl/
[graphviz]: https://www.graphviz.org/

# Testing

To run tests you have to compile them first and then run ctest:

```bash
cmake .. -DBUILD_TESTING=ON
make
ctest
```

Remember to run CMake again whenever a new test is added. CMake generates the rules to
compile the tests and run them. Unfortunatelly, CMake is not triggered when new
tests/foldersr are added to the `test` folder.
