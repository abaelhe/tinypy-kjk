#!/usr/bin/env python
import os, os.path, sys, shutil

SNAPDIR = ".snaps"

def finduniquedir():
    for n in range(9999):
        p = os.path.join(SNAPDIR, "%04d" % n)
        if not os.path.exists(p): return p
    print "Couldn't find a unique directory for snapshots. All taken?"
    sys.exit(1)

def mkdir(p): if not os.path.exists(p): os.makedirs(p)

def main():
    d = finduniquedir()
    print "Snapshot dir: %s" % d
    files = ["vmsmall", "vmsmallstripped"]
    if os.path.exists("vmsmall.exe"): # handle cygwin as well
        files = ["vmsmall.exe", "vmsmallstripped.exe"]
    for f in files:
        dst = os.path.join(d, f)
        mkdir(d)
        print "Copying %s to %s" % (f, dst)
        shutil.copy(f, dst)

if __name__ == "__main__":
    main()

