#!/usr/bin/bash

# This bash command just makes sure to delete the previous ROSS execution and
# passes control to the script that follows

if [ -f "ross.csv" ]; then
  rm ross.csv
fi

if [ -d "output" ]; then
  rm -r output
fi

exec bash -x "$@"
