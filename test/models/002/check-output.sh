#!/usr/bin/bash

diff <(sort "$1/voltage-gid=0.txt") \
     <(sort "$2"/voltage-gid=*.txt) \
   || exit $?

exec diff <(sort "$1/spikes-gid=0.txt") \
          <(sort "$2"/spikes-gid=*.txt)
