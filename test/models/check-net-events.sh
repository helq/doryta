#!/usr/bin/env bash

if [ -F ross.csv ]; then
    echo "ross.csv is not present in directory"
    exit 1
fi

# sanity checking. Column 24 must be "net_events"! (the string)
column_name=$(head -n1 ross.csv | awk -F',' '{ print $24 }')
if [ $column_name != "net_events" ]; then
    echo "column 24 in ross.csv should correspond to 'net_events'," \
        "instead it contains '${column_name}'"
    exit 1
fi

exec diff "$1/net_events_expected.txt" \
          <(awk -F',' '(NR>1) { print $24 }' ross.csv)
