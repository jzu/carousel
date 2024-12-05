#!/bin/bash

# Test whether the connection to the hotspot is active 
# Else, tries to reconnect to the access point
# Address of a NetworkManager hotspot is 10.42.0.1

LOG=/var/tmp/reconnection.log

echo Start `date` >> $LOG
while true
do
  if ! ping -c 1 10.42.0.1 &>/dev/null
  then
    sudo nmcli d wifi rescan
    sudo nmcli d wifi c carousel password carousel &>>$LOG
    echo Reconnection `date` >> $LOG
    ifconfig wlp4s0 >> $LOG
    iwconfig wlp4s0 >> $LOG
  fi
  sleep 10
done
