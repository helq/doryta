#!/usr/bin/bash

diff <(sort "$1/five-neurons-test-voltage-gid=0.txt") \
     <(sort "$2"/five-neurons-test-voltage-gid=*.txt) \
   || exit $?

exec diff <(sort "$1/five-neurons-test-spikes-gid=0.txt") \
          <(sort "$2"/five-neurons-test-spikes-gid=*.txt)
