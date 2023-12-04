#!/bin/bash

while true; do
    clear
    make compile
    cd bin
    ./server $1
    for pid in $(lsof -i tcp:4000 | awk 'NR!=1 {print $2}')
    do
        kill -9 $pid
    done
    sleep 2
done
