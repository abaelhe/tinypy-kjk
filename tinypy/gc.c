/* tp_obj tp_track(TP,tp_obj v) { return v; }
   void tp_grey(TP,tp_obj v) { }
   void tp_full(TP) { }
   void tp_gc_init(TP) { }
   void tp_gc_deinit(TP) { }
   void tp_delete(TP,tp_obj v) { }*/

/* This is tri-color, incremental garbage collector.
*/

int objallocs = 0;
int objallocsfromfreelist = 0;
int objfrees = 0;
int objallocstats[TP_DATA+1] = {0};
int objfreestats[TP_DATA+1] = {0};

typedef struct freelist {
    struct freelist *next;
} freelist;

typedef unsigned char byte;

/* To optimize allocation of doubles, we allocate them in blocks of 15
   with the last 8 bytes for gc grey bit. The total size of the struct is
   then 16*8 = 128 bytes.
*/
struct floatsblock {
    double nums[15];
    byte greybits[8];
};

/* TODO: free objects on objfreelist at some point */
freelist *objfreelist = NULL;
freelist *numfreelist = NULL;

tp_obj obj_alloc(objtype type) {
    tp_obj v;
    if (objfreelist) {
        v = (tp_obj)objfreelist;
        objfreelist = objfreelist->next;
        ++objallocsfromfreelist;
    } else {
        v = (tp_obj) malloc(sizeof(tp_obj_));
    }
    memset(v, 0xdd, sizeof(tp_obj_));
    v->type = type;
    ++objallocs;
    ++objallocstats[type];
    return v;
}

void obj_free(tp_obj v) {
    freelist *entry;
    ++objfrees;
    int type = obj_type(v);
    ++objfreestats[type];
    entry = (freelist*)v;
    entry->next = objfreelist;
    objfreelist = entry;
#if 0
    free(v);
#endif
}

tp_obj tp_number(tp_vm *tp, tp_num v) { 
    tp_obj val = obj_alloc(TP_NUMBER);
    tp_number_val(val) = v;
    val->number.gci = 0;
    val->number.info = (char*)val + offsetof(tp_number_, gci); /* hack for uniform gc */
    return tp_track(tp, val);
}

void tp_grey(tp_vm *tp, tp_obj v) {
    int type = obj_type(v);
    if (type < TP_NUMBER || (!v->gci.data) || *v->gci.data) { return; }
    *v->gci.data = 1;
    if (type == TP_STRING || type == TP_DATA || type == TP_NUMBER) {
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
        tp_grey(tp, v->fnc.val->self);
        tp_grey(tp, v->fnc.val->globals);
    }
}

void tp_reset(tp_vm *tp) {
    int n;
    _tp_list *tmp;
    for (n=0; n<tp->black->len; n++) {
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
        if (v->string.info && !((char*)v->string.info < (char*)v + sizeof(tp_obj_))) {
            tp_free(v->string.info);
        }
        obj_free(v);
        return;
    } else if (type == TP_NUMBER) {
        obj_free(v);
        return;
    } else if (type == TP_LIST) {
        _tp_list_free(tp_list_val(v));
        obj_free(v);
        return;
    } else if (type == TP_FNC) {
        tp_free(v->fnc.val);
        obj_free(v);
        return;
    } else if (type == TP_DICT) {
        _tp_dict_free(tp_dict_val(v));
        obj_free(v);
        return;
    } else if (type == TP_DATA) {
        if (v->data.meta && v->data.meta->free) {
            v->data.meta->free(tp,v);
        }
        tp_free(v->data.info);
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

void _tp_gcinc(tp_vm *tp) {
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
    tp_gcinc(tp);
    tp_grey(tp, v);
    return v;
}

