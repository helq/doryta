#!/usr/bin/bash

# We sort the expected output again because `sort` is different on each
# architecture. The same goes for the wildcard character (*); in principle,
# instead of the wildcard character we could use `{0,1}` but old versions of
# bash behave differently with that input.

diff <(sort "$1/spikes-gid=0.txt") \
     <(sort "$2"/spikes-gid=*.txt) \
   || exit $?

exec diff <(sort "$1/voltage-gid=0.txt") \
     <(sort "$2"/voltage-gid=*.txt)
