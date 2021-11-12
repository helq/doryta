#!/usr/bin/bash

diff <(sort "$1"/fully-connected-layer-spikes-gid=*.txt) \
     <(sort "$2"/fully-connected-layer-spikes-gid=*.txt) \
   || exit $?

exec diff <(sort "$1"/fully-connected-layer-voltage-gid=*.txt) \
          <(sort "$2"/fully-connected-layer-voltage-gid=*.txt)
