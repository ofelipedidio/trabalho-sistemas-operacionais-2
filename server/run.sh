#!/bin/bash

make compile
cd bin
clear
./server $1
