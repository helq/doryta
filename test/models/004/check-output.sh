#!/usr/bin/bash

# Not checking voltage output because it's too large and it doesn't differ
# much from smaller tests (with 5 and 50 neurons)

exec diff "$1/spikes-gid=0.txt" \
          <(sort "$2"/spikes-gid=*.txt)
