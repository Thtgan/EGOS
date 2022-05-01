#!/bin/bash

#$1: Root directory of inject files
#$2: Path to inject tool
#$3: Path to image to inject
#$4: Device name of image
#$5: Base of device
INJECT_TOOL_OPTION="-i $3 -d $4 -b $5"
INJECT_DIRS=$(find $1 -type d | sort)
INJECT_FILES=$(find $1 -type f | sort)

$2 $INJECT_TOOL_OPTION -m deploy

INJECT_PATH_PREFIX_LENGTH=${#1}

for dir in $INJECT_DIRS; do
    if [ $dir == $1 ]; then
        continue
    fi

    pathInside=${dir:$INJECT_PATH_PREFIX_LENGTH}
    $2 $INJECT_TOOL_OPTION -p $(dirname $pathInside) -f $(basename $pathInside) -m addDirectory
done

for file in $INJECT_FILES; do
    pathInside=${file:$INJECT_PATH_PREFIX_LENGTH}
    $2 $INJECT_TOOL_OPTION -p $(dirname $pathInside) -f $file -m addFile
done