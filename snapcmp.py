#!/usr/bin/env python
import os, os.path, string, symbolstatsgcc
from symbolstatsgcc import show_biggest_for_exe, syms_diff

SNAPDIR = ".snaps"

def main():
    show_biggest_for_exe("build/vmsmall")

if __name__ == "__main__":
    main()
