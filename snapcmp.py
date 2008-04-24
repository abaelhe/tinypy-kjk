#!/usr/bin/env python
import os, os.path, sys, shutil, string

SNAPDIR = ".snaps"

SYM_NAME, SYM_ADDR, SYM_SIZE, SYM_TYPE, SYM_LINE = range(5)

SYMS_TO_FILTER = ["__size_of_stack_reserve__", "__size_of_heap_reserve__", "__size_of_stack_commit__"]

# Returns a list of tuples (symbolname, symboladdr, symbolsize, symboltype) based on parsing
# the output of nm -n <file>
def parsenmout(nmout):
    out = []
    n = 0
    for l in string.split(nmout, "\n"):
        l = l.strip()
        if "U .data" in l: continue
        if len(l) == 0: continue
        #print l
        (addr, kind, name) = string.split(l, " ")
        addr = int(addr, 16)
        out.append([name, addr, 0, kind, l])
        if n > 0:
            out[n-1][SYM_SIZE] = addr - out[n-1][SYM_ADDR]
        n = n + 1
    return out

def load(f):
    fo = open(f, "rb")
    d = fo.read()
    fo.close()
    return d

def showzerosized():
    for (o, n) in zip(out, range(len(out))):
        if o[SYM_SIZE] == 0:
            print "%s has size 0" % o[SYM_LINE]
            if n > 0:
                print "%s is previous" % out[n-1][SYM_LINE]
            else:
                print "this is the first line"
            print

def main():
    f = "vmsmall.exe"
    fs = "vmsmallstripped.exe"
    fsize = os.path.getsize(fs)
    os.system("nm -n %s >tmp.txt" % f)
    nmout = load("tmp.txt")
    out = parsenmout(nmout)
    # filter out those that are 0 in size and are on a list of symbols to ignore
    out = [o for o in out if o[SYM_SIZE] > 0 and o[SYM_NAME] not in SYMS_TO_FILTER]
    out.sort(lambda y,x: cmp(x[SYM_SIZE], y[SYM_SIZE]));
    for (o,l) in zip(out, range(15)):
        print "size: %5d %s" % (o[SYM_SIZE], o[SYM_LINE])
    print "total size: %d" % fsize

if __name__ == "__main__":
    main()
