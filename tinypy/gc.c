/* tp_obj tp_track(TP,tp_obj v) { return v; }
   void tp_grey(TP,tp_obj v) { }
   void tp_full(TP) { }
   void tp_gc_init(TP) { }
   void tp_gc_deinit(TP) { }
   void tp_delete(TP,tp_obj v) { }*/

/* This is tri-color, incremental garbage collector.
*/

/* Ideas for improvements:
 - gather stats for double and cache few more frequently used doubles (0, 1)
 - use fixed pointers for none, tp_number(0), tp_number(1) (and maybe some other
   numbers)
*/

void tp_gcinc(tp_vm *tp);

/* TODO: add a static assert that u32 really is 4 bytes */
typedef unsigned long u32;

int objallocs = 0;
int objallocsfromfreelist = 0;
int objfrees = 0;
int objallocstats[TP_DATA+1] = {0};
int objfreestats[TP_DATA+1] = {0};

typedef struct freelist {
    struct freelist *next;
} freelist;

#if 0
typedef struct objfreelist {
    freelist *head;
    int len;
    int limit;
};

struct objfreelist[TP_DATA+1] = {
    {0, 0, 0}, /* TP_NONE */
    {0, 0, 0}, /* TP_NUMBER */
}
#endif

/* We allocate doubles in blocks of NUMS_IN_BLOCK. We choose this value so
   that the final size of numsblock structure is a power of 2 so that we can
   get from the address of double to numsblock by masking lower bits.
   Part of the block is also GC data - 1 bit per double.
   */
#define NUMS_IN_BLOCK 15
/* need just one bit per number, one word is 32 bits */
#define NUMS_GC_WORDS 1
#define NUMS_PADDING_WORDS 1

/* Size is NUMS_IN_BLOCK * 8 + 4 * (NUMS_GC_WORDS + NUMS_PADDING_WORDS) ==
   15*8 + 4*2 = 16*8 = 128, so we need to mask lower 7 bits */
#define numsblock_from_addr(o) (numsblock*)((long)o & 0x7f)

typedef struct numsblock {
    double nums[NUMS_IN_BLOCK];
    u32 greybits[NUMS_GC_WORDS];
    u32 padding[NUMS_PADDING_WORDS];
} numsblock;

/* TODO: free objects on objfreelist at some point */
freelist *objfreelist = NULL;
freelist *numfreelist = NULL;

int tp_inline isbitset(u32 bit, u32 *bitvec) {
    u32 tmp;
    u32 word = bit / 32;
    bit = bit % 32;
    bitvec += word;
    tmp = *bitvec & (1 << bit);
    return 0 != tmp;
}

void tp_inline setbit(u32 bit, u32 *bitvec) {
    u32 word = bit / 32;
    bit = bit % 32;
    bitvec += word;
    *bitvec |= (1 << bit);
}

/* Sets the bit and returns 0 if the bit is not set.
   Returns 1 if bit is already set */
int tp_inline setbitmaybe(u32 bit, u32 *bitvec) {
    u32 tmp;
    u32 word = bit / 32;
    bit = bit % 32;
    bitvec += word;
    tmp = *bitvec & (1 << bit);
    if (0 != tmp)
        return 1;
    *bitvec |= (1 << bit);
    return 0;
}

void tp_inline clearbit(u32 bit, u32 *bitvec) {
    u32 word = bit / 32;
    bit = bit % 32;
    bitvec += word;
    *bitvec &= ~(1 << bit);
}

tp_inline void numfreelistput(freelist* entry) {
    entry->next = numfreelist;
    numfreelist = entry;
}

tp_inline void objfreelistput(freelist* entry) {
    entry->next = objfreelist;
    objfreelist = entry;
}

tp_inline void numcleargcbit(tp_obj v)
{
    numsblock *blk = numsblock_from_addr(v);
    double *num = (double*)untag_ptr(v);
    u32 idx = num - &(blk->nums[0]);
    assert(idx < NUMS_IN_BLOCK);
    clearbit(idx, blk->greybits);
}

tp_inline void numsetgcbit(tp_obj v)
{
    numsblock *blk = numsblock_from_addr(v);
    double *num = (double*)untag_ptr(v);
    u32 idx = num - &(blk->nums[0]);
    assert(idx < NUMS_IN_BLOCK);
    setbit(idx, blk->greybits);
}

tp_inline int numsetgcbitmaybe(tp_obj v)
{
    numsblock *blk = numsblock_from_addr(v);
    double *num = (double*)untag_ptr(v);
    u32 idx = num - &(blk->nums[0]);
    assert(idx < NUMS_IN_BLOCK);
    return setbitmaybe(idx, blk->greybits);
}

#define NUM_ALLOC_OPT 0


tp_obj alloc_num() {
    tp_obj v;
    int i;
    if (!numfreelist) {
        struct numsblock *nums = (numsblock*)malloc(sizeof(numsblock));
        for (i = 0; i < 15; i++) {
            numfreelistput((freelist*)&nums[i]);
        }
    } else {
        ++objallocsfromfreelist;
    }
    ++objallocs;
    ++objallocstats[TP_NUMBER];
    v = (tp_obj)numfreelist;
    numfreelist = numfreelist->next;
#if !NUM_ALLOC_OPT
    numcleargcbit(v);
#endif
    return tag_ptr(v, tagNumber);
}

void free_num(tp_obj v) {
    ++objfrees;
    ++objfreestats[TP_NUMBER];
    numfreelistput((freelist*)v);
}

tp_obj obj_alloc(objtype type) {
    tp_obj v;
    if (TP_NUMBER == type)
        return alloc_num();

    if (objfreelist) {
        v = (tp_obj)objfreelist;
        objfreelist = objfreelist->next;
        ++objallocsfromfreelist;
    } else {
        v = (tp_obj)malloc(sizeof(tp_obj_));
    }
    memset(v, 0xdd, sizeof(tp_obj_));
    v->type = type;
    ++objallocs;
    ++objallocstats[type];
    return tag_ptr(v, tagObject);
}

/* TODO: limit the amount of stuff on free list */
void obj_free(tp_obj v) {
    if (is_number(v))
        free_num(v);
    else {
        int type = obj_type(v);
        ++objfreestats[type];
        ++objfrees;
        objfreelistput((freelist*)untag_ptr(v));
    }
}

void grey_num(tp_vm *tp, tp_obj v) {
    if (numsetgcbitmaybe(v)) {
        _tp_list_appendx(tp, tp->black, v);
    }
}

tp_inline void tp_track_non_str(tp_vm *tp, tp_obj v) {
    tp_gcinc(tp);
    tp_grey(tp, v);
}

tp_obj tp_number(tp_vm *tp, tp_num v) {
    tp_obj val = alloc_num();
#if  NUM_ALLOC_OPT
    numsetgcbit(v);
    _tp_list_appendx(tp, tp->black, v);
#else
    tp_track_non_str(tp, val);
#endif
    return val;
}

void tp_grey(tp_vm *tp, tp_obj v) {
    int type;

    if (is_number(v)) {
        grey_num(tp, v);
        return;
    }

    if (None == v)
        return;

    /* TODO: this should go away once all objects have gci.data */
    if ((!v->gci.data) || *v->gci.data) { 
        return; 
    }

    *v->gci.data = 1;
    type = obj_type(v);
    if (type == TP_STRING || type == TP_DATA) {
        /* doesn't reference other objects */
        _tp_list_appendx(tp, tp->black, v);
        return;
    }
    _tp_list_appendx(tp, tp->grey, v);
}

void tp_follow(tp_vm *tp, tp_obj v) {
    int type = obj_type(v);
    if (type == TP_LIST) {
        int n;
        for (n=0; n < tp_list_val(v)->len; n++) {
            if (tp_list_val(v)->items[n])
                tp_grey(tp, tp_list_val(v)->items[n]);
        }
    }
    if (type == TP_DICT) {
        int i;
        for (i=0; i < tp_dict_val(v)->len; i++) {
            int n = _tp_dict_next(tp, tp_dict_val(v));
            tp_grey(tp, tp_dict_val(v)->items[n].key);
            tp_grey(tp, tp_dict_val(v)->items[n].val);
        }
    }
    if (type == TP_FNC) {
        tp_grey(tp, tp_fnc_val(v)->self);
        tp_grey(tp, tp_fnc_val(v)->globals);
    }
}

void tp_reset(tp_vm *tp) {
    int n;
    _tp_list *tmp;
    for (n=0; n<tp->black->len; n++) {
        /* TODO: distinguish doubles ? */
        *tp->black->items[n]->gci.data = 0;
    }
    tmp = tp->white; 
    tp->white = tp->black; 
    tp->black = tmp;
}

void tp_gc_init(tp_vm *tp) {
    tp->white = _tp_list_new();
    tp->strings = _tp_dict_new();
    tp->grey = _tp_list_new();
    tp->black = _tp_list_new();
    tp->steps = 0;
}

void tp_gc_deinit(tp_vm *tp) {
    _tp_list_free(tp->white);
    _tp_dict_free(tp->strings);
    _tp_list_free(tp->grey);
    _tp_list_free(tp->black);
}

void tp_delete(tp_vm *tp, tp_obj v) {
    int type = obj_type(v);
    /* checks are ordered by frequency of allocation */
    if (type == TP_STRING) {
        tp_string_ *s = &v->string;
        if (s->info && !((char*)s->info < (char*)v + sizeof(tp_obj_))) {
            tp_free(s->info);
        }
        obj_free(v);
        return;
    } else if (type == TP_NUMBER) {
        free_num(v);
        return;
    } else if (type == TP_LIST) {
        _tp_list_free(tp_list_val(v));
        obj_free(v);
        return;
    } else if (type == TP_FNC) {
        tp_free(tp_fnc_val(v));
        obj_free(v);
        return;
    } else if (type == TP_DICT) {
        _tp_dict_free(tp_dict_val(v));
        obj_free(v);
        return;
    } else if (type == TP_DATA) {
        tp_data_ *d = &v->data;
        if (d->meta && d->meta->free) {
            d->meta->free(tp,v);
        }
        tp_free(d->info);
        obj_free(v);
        return;
    }
    tp_raise(,"tp_delete(%s)",STR(v));
}

void tp_collect(tp_vm *tp) {
    int n;
    for (n=0; n<tp->white->len; n++) {
        tp_obj r = tp->white->items[n];
        if (*r->gci.data) { 
            continue; 
        }
        if (obj_type(r) == TP_STRING) {
            /* this can't be moved into tp_delete, because tp_delete is
               also used by tp_track_s to delete redundant strings */
            _tp_dict_del(tp, tp->strings, r, "tp_collect");
        }
        tp_delete(tp, r);
    }
    tp->white->len = 0;
    tp_reset(tp);
}

static void _tp_gcinc(tp_vm *tp) {
    tp_obj v;
    if (!tp->grey->len) { 
        return; 
    }
    v = _tp_list_pop(tp, tp->grey, tp->grey->len-1, "_tp_gcinc");
    tp_follow(tp, v);
    _tp_list_appendx(tp,tp->black,v);
}

void tp_full(tp_vm *tp) {
    while (tp->grey->len) {
        _tp_gcinc(tp);
    }
    tp_collect(tp);
    tp_follow(tp, tp->root);
}

void tp_gcinc(tp_vm *tp) {
    tp->steps += 1;
    if (tp->steps < TP_GCMAX || tp->grey->len > 0) {
        _tp_gcinc(tp); 
        _tp_gcinc(tp);
    }
    if (tp->steps < TP_GCMAX || tp->grey->len > 0) { 
        return; 
    }
    tp->steps = 0;
    tp_full(tp);
    return;
}

tp_obj tp_track(tp_vm *tp, tp_obj v) {
    if (obj_type(v) == TP_STRING) {
        int i = _tp_dict_find(tp, tp->strings, v);
        if (i != -1) {
            tp_delete(tp, v);
            v = tp->strings->items[i].key;
            tp_grey(tp,v);
            return v;
        }
        _tp_dict_setx(tp, tp->strings, v, True);
    }
    tp_track_non_str(tp, v);
    return v;
}

