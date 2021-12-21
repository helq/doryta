#!/usr/bin/bash

diff <(sort "$1"/*-spikes-gid=*.txt) \
     <(sort "$2"/*-spikes-gid=*.txt) \
   || exit $?

exec diff <(sort "$1"/*-stats-gid=*.txt) \
          <(sort "$2"/*-stats-gid=*.txt)
