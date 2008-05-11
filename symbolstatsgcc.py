import sys, os, os.path, string

# Helper functions for calculating changes in symbols by using nm on
# unstripped executables

def load(f):
    fo = open(f, "rb")
    d = fo.read()
    fo.close()
    return d

class Symbol(object):
    def __init__(self, name, addr, kind, size, line):
        self.name = name
        self.addr = addr
        # nm -P has size of the symbol listed only on linux, on mac it's always
        # 0, so self.sizefromnm recodrds that but we also calculate the size
        # by substracting address from previous symbol
        self.sizefromnm = size
        self.calcsize = 0
        self.kind = kind
        self.line = line

    def size(self):
        if self.sizefromnm != 0:
            return self.sizefromnm
        return self.calcsize

SYMS_TO_FILTER = ["__size_of_stack_reserve__", "__size_of_heap_reserve__", "__size_of_stack_commit__"]
def filter_sym(sym):
    if sym.size() == 0: return True
    if sym.name in SYMS_TO_FILTER: return True
    return False

# Returns a list of Symbol objects based on parsing the output of nm -P -n <file>
def parse_nm_out(nmout, fsize):
    syms = []
    n = 0
    for l in string.split(nmout, "\n"):
        l = l.strip()
        if 0 == len(l): continue
        parts = string.split(l, " ")
        # skip lines like "__fxstat@@GLIBC_2.0 U" generted by nm on linux
        if 2 == len(parts): continue
        if 3 == len(parts):
            (name, kind, addr) = parts
            size = "0"
        else:
            if 4 != len(parts):
                print l
                print len(parts)
            assert(4 == len(parts))
            (name, kind, addr, size) = parts
        addr = int(addr, 16)
        size = int(size, 16)
        sym = Symbol(name, addr, kind, size, l)
        syms.append(sym)
        if n > 0:
            prevsym = syms[n-1]
            prevsym.calcsize = addr - prevsym.addr
        n = n + 1
    last = syms[-1]
    last.calcsize = fsize - last.addr

    # for symbols with the same name, combine them into one symbol
    # TODO: can I do something more intelligent about 
    # __Z41__static_initialization_and_destruction_0ii symbols 
    # (e.g. create a unique name by combining with the name of the previous/next symbol?)
    combined = []
    symname_to_sym = {}
    for sym in syms:
        if sym.name in symname_to_sym:
            symname_to_sym[sym.name].calcsize += sym.calcsize
            symname_to_sym[sym.name].sizefromnm += sym.sizefromnm
        else:
            symname_to_sym[sym.name] = sym
            combined.append(sym)
    filtered = [sym for sym in combined if not filter_sym(sym)]
    return filtered

def syms_for_exe(f):
    fsize = os.path.getsize(f)
    os.system("nm -n -P %s >nm_out_tmp.txt" % f)
    nm_out = load("nm_out_tmp.txt")
    res = parse_nm_out(nm_out, fsize)
    # TODO: remove the file
    return res

(ADDED, REMOVED, CHANGED) = ("ADDED", "REMOVED", "CHANGED")
class SymbolDiff(object):
    def __init__(self, name, symtype, size):
        self.name = name
        self.type = symtype
        self.size = size

def syms_diff(syms1, syms2):
    syms1_by_name = {}
    for s in syms1:
        syms1_by_name[s.name] = s
    syms2_by_name = {}
    for s in syms2:
        syms2_by_name[s.name] = s
        diffs = []
    for s2 in syms2:
        if s2.name not in syms1_by_name:
            diffs.append(SymbolDiff(s2.name, ADDED, s2.size()))
        else:
            s1 = syms1_by_name[s2.name]
            if s2.size() != s1.size():
                diffs.append(SymbolDiff(s2.name, CHANGED, s2.size() - s1.size()))
    for s1 in syms1:
        if s1.name not in syms2_by_name:
            diffs.append(SymbolDiff(s1.name, REMOVED, -s1.size()))
    return diffs

def diff_dump_helper(diffs, difftype):
    if len(diffs) == 0: return
    print difftype
    for d in diffs: print "%5d: %s" % (d.size(), d.name)

def diff_dump(diffs):
    total_change = 0
    for d in diffs: total_change += d.size()
    print "Total change in size: %d" % total_change
    added = [d for d in diffs if d.type == ADDED]
    removed = [d for d in diffs if d.type == REMOVED]
    changed = [d for d in diffs if d.type == CHANGED]
    diff_dump_helper(added, "Added:")
    diff_dump_helper(removed, "Removed:")
    diff_dump_helper(changed, "Changed:")
    print "Total change in size: %d" % total_change

def diff_exes(exe1, exe2):
    syms1 = syms_for_exe(exe1)
    syms2 = syms_for_exe(exe2)
    diffs = syms_diff(syms1, syms2)
    diff_dump(diffs)

def show_biggest_for_exe(f):
    syms = syms_for_exe(f)
    syms.sort(lambda y,x: cmp(x.size(), y.size()));
    for (sym,l) in zip(syms, range(25)):
        print "size: %5d %s" % (sym.size(), sym.line)
        def showzerosized(syms):
            for (sym, n) in zip(syms, range(len(syms))):
                if sym.size() == 0:
                    print "%s has size 0" % sym.line
                    if n > 0:
                        print "%s is previous" % syms[n-1].lines
                    else:
                        print "this is the first line"
                    print
