#!/bin/bash

# Resize all pictures to W=1920
# Rotate according to Exif data if needed
# Original files in /tmp/[0-9]/  (must exist)
# Converted files in $PWD/[0-9]/ (must not exist)

export LANG=C
export LC_ALL=C

if ! ls /tmp/[0-9] &>/dev/null
then
  echo Source directories not present: aborting 1>&2
  exit 1
fi

if ls [0-9] &>/dev/null
then
  echo Destination directories present: aborting 1>&2
  exit 2
fi

if ! exiftool -ver >/dev/null
then
  echo Install with '"sudo apt install exiftool"' 1>&2
  exit 3
fi

for D in `seq 0 9`
do
  if [ -d /tmp/$D ]
  then
    mkdir -p $D
    ls /tmp/$D \
    | while read I
      do 
        unset ORIENT
        exiftool "/tmp/$D/$I" \
        | grep -q 'Orientation.*Rotate' && \
            ORIENT=`exiftool "/tmp/$D/$I" \
                    | grep Orientation \
                    | sed 's/.*Rotate \(.*\) CW/-rotate \1/'`
        convert "/tmp/$D/$I" $ORIENT -resize 1920 -quality 82 -unsharp 0.25x0.25+8+0.065 "$D/$I"
      done
  fi
done
