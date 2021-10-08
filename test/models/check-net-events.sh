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

net_events_expected=$(cat "$1/net_events_expected.txt")
net_events_computed=$(tail -n1 ross.csv | awk -F',' '{ print $24 }')
if [ $net_events_computed -ne $net_events_expected ]; then
    echo "The number of expected Net Events ${net_events_expected}" \
        "does not correspond to the computed Net Events ${net_events_computed}"
    exit 1
fi
