#!/usr/bin/env python
import os, sys

def do_cmd(cmd):
    print cmd
    r = os.system(cmd)
    if r:
        print 'exit',r
        sys.exit(r)

def buildme():
    do_cmd("gcc -Wall -g -Os vmmain.c -lm -o vmsmall")
    if os.path.exists("vmsmall.exe"):
        do_cmd("strip vmsmall.exe -o vmsmallstripped.exe")
    else:
        do_cmd("strip vmsmall -o vmsmallstripped")
    do_cmd("ls -la vmsmall*")

if __name__ == '__main__':
    buildme()
