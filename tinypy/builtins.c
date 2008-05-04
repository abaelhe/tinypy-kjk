tp_obj tp_print(TP) {
    int n = 0;
    tp_obj e;
    TP_LOOP(e)
        if (n) { printf(" "); }
        printf("%s",STR(e));
        n += 1;
    TP_END;
    printf("\n");
    return None;
}

tp_obj tp_bind(TP) {
    tp_obj r = TP_OBJ();
    tp_obj self = TP_OBJ();
    return tp_fnc_new(tp, r->fnc.ftype|2, r->fnc.fval, self, r->fnc.val->globals);
}

tp_obj tp_min(TP) {
    tp_obj r = TP_OBJ();
    tp_obj e;
    TP_LOOP(e)
        if (tp_cmp(tp,r,e) > 0) { r = e; }
    TP_END;
    return r;
}

tp_obj tp_max(TP) {
    tp_obj r = TP_OBJ();
    tp_obj e;
    TP_LOOP(e)
        if (tp_cmp(tp,r,e) < 0) { r = e; }
    TP_END;
    return r;
}

tp_obj tp_copy(tp_vm *tp) {
    tp_obj r = TP_OBJ();
    int type = obj_type(r);
    if (type == TP_LIST) {
        return _tp_list_copy(tp, r);
    } else if (type == TP_DICT) {
        return _tp_dict_copy(tp, r);
    }
    tp_raise(None, "tp_copy(%s)", STR(r));
}


tp_obj tp_len_(TP) {
    tp_obj e = TP_OBJ();
    return tp_len(tp,e);
}


tp_obj tp_assert(TP) {
    int a = TP_NUM();
    if (a) { return None; }
    tp_raise(None,"%s","assert failed");
}

tp_obj tp_range(tp_vm *tp) {
    int a,b,c,i;
    tp_obj r = tp_list(tp);
    switch (__params->list.val->len) {
        case 1: 
            a = 0; 
            b = TP_NUM(); 
            c = 1; 
            break;
        case 2:
        case 3: 
            a = TP_NUM(); 
            b = TP_NUM(); 
            c = tp_number_val(TP_DEFAULT(tp_number(tp, 1))); 
            break;
        default: return r;
    }
    if (c != 0) {
        for (i=a; (c>0) ? i<b : i>b; i+=c) {
            _tp_list_append(tp, r->list.val, tp_number(tp, i));
        }
    }
    return r;
}

tp_obj tp_system(tp_vm *tp) {
    char *s = TP_STR();
    int r = system(s);
    return tp_number(tp, r);
}

tp_obj tp_istype(tp_vm *tp) {
    tp_obj v = TP_OBJ();
    char *t = TP_STR();
    if (strcmp("string",t) == 0) { return tp_number(tp, obj_type(v) == TP_STRING); }
    if (strcmp("list",t) == 0) { return tp_number(tp, obj_type(v) == TP_LIST); }
    if (strcmp("dict",t) == 0) { return tp_number(tp, obj_type(v) == TP_DICT); }
    if (strcmp("number",t) == 0) { return tp_number(tp, obj_type(v) == TP_NUMBER); }
    tp_raise(None,"is_type(%s,%s)",STR(v),t);
}

tp_obj tp_float(tp_vm *tp) {
    tp_obj v = TP_OBJ();
    int ord = tp_number_val(TP_DEFAULT(tp_number(tp, 0)));
    int type = obj_type(v);
    if (type == TP_NUMBER) { return v; }
    if (type == TP_STRING) {
        if (strchr(STR(v),'.')) { 
            return tp_number(tp, atof(STR(v))); 
        }
        return(tp_number(tp, strtol(STR(v),0,ord)));
    }
    tp_raise(None,"tp_float(%s)",STR(v));
}

tp_obj tp_save(tp_vm *tp) {
    char *fname = TP_STR();
    tp_obj v = TP_OBJ();
    FILE *f;
    f = fopen(fname,"wb");
    if (!f) { tp_raise(None,"tp_save(%s,...)",fname); }
    fwrite(v->string.val, v->string.len,1,f);
    fclose(f);
    return None;
}

tp_obj tp_load(tp_vm *tp) {
    FILE *f;
    long l;
    tp_obj r;
    char *s;
    char *fname = TP_STR();
    struct stat stbuf; 
    stat(fname, &stbuf); 
    l = stbuf.st_size;
    f = fopen(fname,"rb");
    if (!f) { 
        tp_raise(None,"tp_load(%s)",fname); 
    }
    r = tp_string_t(tp,l);
    s = r->string.val;
    fread(s,1,l,f);
    fclose(f);
    return tp_track(tp,r);
}


tp_obj tp_fpack(tp_vm *tp) {
    tp_num v = TP_NUM();
    tp_obj r = tp_string_t(tp,sizeof(tp_num));
    *(tp_num*)r->string.val = v; 
    return tp_track(tp, r);
}

tp_obj tp_abs(tp_vm *tp) {
    return tp_number(tp, fabs(tp_number_val(tp_float(tp))));
}

tp_obj tp_int(tp_vm *tp) {
    return tp_number(tp, (long)tp_number_val(tp_float(tp)));
}

tp_num _roundf(tp_num v) {
    tp_num av = fabs(v); tp_num iv = (long)av;
    av = (av-iv < 0.5?iv:iv+1);
    return (v<0?-av:av);
}

tp_obj tp_round(tp_vm *tp) {
    return tp_number(tp, _roundf(tp_number_val(tp_float(tp))));
}

tp_obj tp_exists(tp_vm *tp) {
    char *s = TP_STR();
    struct stat stbuf;
    return tp_number(tp, !stat(s,&stbuf));
}
tp_obj tp_mtime(tp_vm *tp) {
    char *s = TP_STR();
    struct stat stbuf;
    if (!stat(s,&stbuf)) { 
        return tp_number(tp, stbuf.st_mtime); 
    }
    tp_raise(None,"tp_mtime(%s)",s);
}
