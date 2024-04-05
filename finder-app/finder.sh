#!/bin/sh

usage() { echo "Usage: $0 filedir searchstr" 1>&2; exit 1; }

# $1 is the filedirectoryto search
# $2 is the string to search for in the directory

# Argument checking
if [ $# -lt 2 ]; then
    echo "Too few arguments passed in"
    usage
    exit 1
elif [ $# -gt 2 ]; then
    echo "Too many arguments passed in"
    usage
    exit 1
elif [ ! -d $1 ]; then
    echo $1
    echo "File directory does not exist or is not a directory"
    usage
    exit 1
fi

# Get file + matching line count
file_count="$(find $1 -type f | wc -l)"
# Line count may include multiple matches of the same string within a line
line_count="$(grep -r $2 $1 | wc -l)"

echo "The number of files are $file_count and the number of matching lines are $line_count"
