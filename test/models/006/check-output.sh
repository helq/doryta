#!/usr/bin/bash

diff <(sort "$1"/spikes-gid=*.txt) \
     <(sort "$2"/spikes-gid=*.txt) \
   || exit $?

exec diff <(sort "$1"/voltage-gid=*.txt) \
          <(sort "$2"/voltage-gid=*.txt)
