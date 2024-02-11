#!/bin/sh

if [ "$#" -ne 2 ] 
then
    echo "Usage: <writefile> <writestr>"
    exit 1
fi

writefile="$1"
writestr="$2"

dirpath=$(dirname "$writefile")
mkdir -p "$dirpath"

if echo "$writestr" > "$writefile"
then
    echo "Succesfully written to '$writefile'."
else
    echo "Error: Failed to write to '$writefile'."
    exit 1
fi
