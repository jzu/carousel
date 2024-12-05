#!/bin/bash

# Global client wrapper
# To be called at user session startup

./connect_carousel.sh &
./monitor.sh &> /var/tmp/monitor.log &
./udpkeyboard
