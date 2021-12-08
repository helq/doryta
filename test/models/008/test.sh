#!/usr/bin/bash

exec mpirun -np 2 "$2" --synch=3 --spike-driven
