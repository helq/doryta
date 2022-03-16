#!/usr/bin/bash

# Not checking voltage because spike-driven doesn't record all voltages

exec diff <(sort "$2"/spike-driven-test/spikes-gid=*.txt) \
          <(sort "$2"/needy-test/spikes-gid=*.txt)
