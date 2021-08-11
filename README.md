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

An example of running in one core or two:

```bash
cd build
bin/doryta --help
mpirun -np 2 bin/doryta --sync=2 --batch=1 --model=../data/models/one_neuron.neuro --spikes=../data/models/one_spike.spike
```

Inside the directory `output/` there will be two files: `output_spikes.spikes` and
`voltages.json`.

# Documentation

To generate the documentation, install [doxygen][] and dot (included in [graphviz][]), and
then run:

```bash
doxygen docs/Doxyfile
```

The documentation will be stored in `docs/html`.

[doxygen]: https://www.doxygen.nl/
[graphviz]: https://www.graphviz.org/
