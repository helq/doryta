# Documentation

To generate the documentation, install [doxygen][] and dot (included in [graphviz][]), and
then run:

```bash
doxygen docs/Doxyfile
```

The documentation will be stored in `docs/html`.

[doxygen]: https://www.doxygen.nl/
[graphviz]: https://www.graphviz.org/

# Notes on debugging

Debugging is easish again! (Debugging MPI code seemed to be nasty at first, but it doesn't
have to be.)

First of all, make sure you are compiling in debug mode:

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
```

There are two options:

1. Running the code in a single core

    ```bash
    gdb --args src/doryta --synch=3 --end=1
    ```

    Or use Visual Studio Code. It's great.

2. Running the code in several mpi ranks. (You have to install [tmux-mpi][] and dtrace.)

    ```bash
    export TMUX_MPI_MPIRUN="mpiexec --mca opal_warn_on_missing_libcuda 0"
    export TMUX_MPI_MODE=pane
    export TMUX_MPI_SYNC_PANES=1
    tmux-mpi 2 gdb --args src/doryta --synch=3 --end=1

    # In another terminal run
    tmux attach -t tmux-mpi
    # Tips (inside of tmux):
    # - Ctr+b :setw synchronize-panes
    ```

[tmux-mpi]: https://github.com/wrs20/tmux-mpi

# Thoughts on various time-related concerns in ROSS

+ In ROSS, it is not possible to schedule an event for a specific time, instead an event is
    scheduled to an offset time in the future. This presents a dilemma: if one wants to
    make sure that an event occurs at precisely a given point in time, one has to
    substract the future time to the current time and hope that adding them up will give
    you back the future time you wanted, but this is not warrantied, but one should be
    certain that the timestamp will be correct.

    To determine the offset given a timestamp and `tw_now()` one can do:
    `offset = tw_now() - timestamp`. The question is whether
    `offset + tw_now() == timestamp`. Well, this seems to be true unless one of the values
    is `NaN`.

+ _On why heartbeat offsets must be a power of 2_:
    Heartbeats should happen always at the same timestamps whether _needy_ or
    _spike-driven_ modes are used. In needy mode, the heartbeats are always fired one
    after the other and their timestamps are thus the result of repeated addition of their
    heartbeat offsets. In spike-driven mode, we have to compute when the next heartbeat
    timestamp lies and _program_ it for that specific timestamp.

    It is easy to compute and be certain of the timestamp for all heartbeats in the
    "floating point line" if their offset is 1.0. (We can check with ease and be
    certain about whether a floating point number corresponds to a heartbeat timestamp for
    the offset 1.0.) 20012.0 is the timestamp of a heartbeat as well as 4253141.0 is.
    Determining the heartbeat timestamps for any other offset might is fraught to fall
    into floating-point number rounding errors.

    The reason why a heartbeat offset of 1.0 is easy, it's because it is a power of the
    base we are counting on. 1.0 is 10^0 and, more importantly, 2^0. Any other power of 2
    is as good as 1.0, unless it's extremely large or small.
