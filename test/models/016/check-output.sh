#!/usr/bin/bash

# Only the first 1200 (20 x 20 x 3) neurons are identical between both models.
# We only really care with GoL about the first 400

# Checking that the number of firings match
diff <(awk '$1 < 400 { print $1 "\t" $5 }' "$2"/gol-20x20/stats-gid=*.txt | sort) \
     <(awk '$1 < 400 { print $1 "\t" $5 }' "$2"/gol-20x20-non-negative/stats-gid=*.txt | sort) \
   || exit $?

# Checking that each firing match
# * `sed` deletes numbers after decimal
# * `awk` only allows for neurons with id < 400 (and decrements the time of
#         spike for hardcoded GoL)
# Both instructions are necessary because the behaviour of the circuits is the
# same globally, but not at a neuron by neuron level
exec diff <(sed 's/\.[^[:blank:]]*//' "$2"/gol-20x20/spikes-gid=0.txt | awk '$1 < 400 { print $1 "\t" ($2 - 1) }' | sort) \
          <(sed 's/\.[^[:blank:]]*//' "$2"/gol-20x20-non-negative/spikes-gid=0.txt | awk '$1 < 400' | sort)
