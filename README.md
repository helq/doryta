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
cmake .. -DCMAKE_INSTALL_PREFIX="$(pwd -P)/"
make install
```

After compiling, you will find the executable under the folder: `build/bin`

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

# Developing

Doryta is C17 (and thus C11) compliant. When doryta is compiled with C extensions, it
makes use of them to further typecheck some expressions (eg, closures as defined in
`src/utils/closure.h` define an opaque pointer `void *` which holds information to be
processed by the closured function, `typeof` is used in here to extract the type of the
data being passed and enforce that the closured function works with the data stored).

To activate the extensions run cmake with:

```bash
cmake .. -DCMAKE_C_EXTENSIONS=ON
```
