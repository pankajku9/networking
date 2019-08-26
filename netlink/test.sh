#!/bin/bash
set -e
set -x

gcc -o ncli nlsocket.c
echo 'screen ./ncli -s 1 -c 10  # Open first screen and launch first script
split -v          # Make second split
focus             # Switch to second split 
screen ./ncli -c 10 # Open second screen and launch second script' > _temp
screen -c _temp
rm -f _temp
rm -f ncli