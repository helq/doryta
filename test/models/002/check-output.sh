#!/usr/bin/bash

diff "$1/five-neurons-test-voltage-gid=0.txt" \
     <(sort "$2"/five-neurons-test-voltage-gid={0,1}.txt) \
   || exit $?

exec diff "$1/five-neurons-test-spikes-gid=0.txt" \
          <(sort "$2"/five-neurons-test-spikes-gid={0,1}.txt)
