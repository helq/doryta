#!/usr/bin/bash

diff <(sort "$2"/gol-middle-spikes/stats-gid=*.txt) \
     <(sort "$2"/gol-random-sched-spikes/stats-gid=*.txt) \
   || exit $?

exec diff <(sort "$2"/gol-middle-spikes/spikes-gid=*.txt) \
          <(sort "$2"/gol-random-sched-spikes/spikes-gid=*.txt) \
