#!/usr/bin/bash

# Not checking voltage output because it's too large and it doesn't differ
# much from smaller tests (with 5 and 50 neurons)

for file in "$2"/*/spikes-gid=*.txt; do
    [ -s "$file" ] && >&2 echo "This network should NEVER produce any output spikes" && exit 1
done

exit 0
