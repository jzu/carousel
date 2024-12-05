#!/bin/bash

# Connects to the Apache server on the hostpot/slideshow
# Get the currently displayed file name
# If it has changed, download the file 
# Display it on the laptop screen background
# -nc ensures no useless download

mkdir -p /tmp/monitor
cd /tmp/monitor
OLD=''
while true
do
  NEW=`wget -q -O - http://carousel:1234/current.txt`
  if [ "$OLD" != "$NEW" ]
  then
    echo "$NEW" >> /tmp/monitor.log
    wget -q -nc "http://carousel:1234/$NEW"
    NEW=`basename "$NEW"`
    touch "$NEW"
    pkill display
    display -window root "$NEW" &
    OLD="$NEW"
  fi
  sleep 1
done
