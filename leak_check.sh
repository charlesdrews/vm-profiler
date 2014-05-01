#!/bin/sh

EXE="profiler"
INPUT=$1
valgrind --leak-check=full ./$EXE $1 > /dev/null
