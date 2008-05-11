#!/bin/sh
gcc -std=c89 -Wall -O tinypy/mymain.c -Itinypy -lm -o build/vmsmall
