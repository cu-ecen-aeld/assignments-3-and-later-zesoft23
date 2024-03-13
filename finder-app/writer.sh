#!/bin/bash

usage() { echo "Usage: $0 writefile writestr" 1>&2; exit 1; }

# $1 is the file to write to
# $2 is the string to write to the file

# Argument checking
if [[ $# -lt 2 ]]; then
    echo "Too few arguments passed in"
    usage
    exit 1
elif [[ $# -gt 2 ]]; then
    echo "Too many arguments passed in"
    usage
    exit 1
fi

# will exit with one if this can't work e.g. writing to a file
# in the root directory as non-root

directory="$(dirname $1)"
if [[ directory ]]; then
    mkdir -p $directory
fi

echo "$2" > $1



