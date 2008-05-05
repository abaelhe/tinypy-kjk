tp_obj tp_string_n(tp_vm *tp, char *v, int n) {
    tp_obj val = obj_alloc(TP_STRING);
    tp_obj valUntagged = untag_ptr(val);
    tp_string_ *s = &valUntagged->string;
    s->info = (struct _tp_string*) ((char*)valUntagged + offsetof(tp_string_, gci)); /* hack for uniform gc */
    s->val = v;
    s->len = n;
    s->gci = 0;
    return tp_track(tp, val);
}

tp_obj tp_string(tp_vm *tp, char *v) {
    return tp_string_n(tp, v, strlen(v));
}

tp_obj tp_string_t(tp_vm *tp, int n) {
    tp_obj r = obj_alloc(TP_STRING);
    tp_str_len(r) = n;
    tp_str_info(r) = tp_malloc(sizeof(_tp_string)+n);
    tp_str_val(r) = tp_str_info(r)->s;
    return r;
}

tp_obj tp_printf(tp_vm *tp, char *fmt, ...) {
    int l;
    tp_obj r;
    char *s;
    va_list arg;
    va_start(arg, fmt);
    l = vsnprintf(NULL, 0, fmt,arg);
    r = tp_string_t(tp, l);
    s = tp_str_val(r);
    va_end(arg);
    va_start(arg, fmt);
    vsprintf(s,fmt,arg);
    va_end(arg);
    return tp_track(tp,r);
}

int _tp_str_index(tp_obj s, tp_obj k) {
    int i=0;
    while ((tp_str_len(s) - i) >= tp_str_len(k)) {
        if (memcmp(tp_str_val(s)+i, tp_str_val(k), tp_str_len(k)) == 0) {
            return i;
        }
        i += 1;
    }
    return -1;
}

tp_obj tp_join(TP) {
    tp_obj delim = TP_OBJ();
    tp_obj val = TP_OBJ();
    int l=0,i;
    tp_obj r;
    char *s;
    for (i=0; i < tp_list_val(val)->len; i++) {
        if (i!=0) { l += tp_str_len(delim); }
        l += tp_str_len(tp_str(tp, tp_list_val(val)->items[i]));
    }
    r = tp_string_t(tp,l);
    s = tp_str_val(r);
    l = 0;
    for (i=0; i < tp_list_val(val)->len; i++) {
        tp_obj e;
        if (i!=0) {
            memcpy(s+l, tp_str_val(delim), tp_str_len(delim));
            l += tp_str_len(delim);
        }
        e = tp_str(tp, tp_list_val(val)->items[i]);
        memcpy(s+l, tp_str_val(e), tp_str_len(e));
        l += tp_str_len(e);
    }
    return tp_track(tp,r);
}

tp_obj tp_string_slice(TP,tp_obj s, int a, int b) {
    tp_obj r = tp_string_t(tp,b-a);
    char *m = tp_str_val(r);
    memcpy(m, tp_str_val(s)+a,b-a);
    return tp_track(tp,r);
}

tp_obj tp_split(TP) {
    tp_obj v = TP_OBJ();
    tp_obj d = TP_OBJ();
    tp_obj r = tp_list(tp);
    
    int i;
    while ((i=_tp_str_index(v,d))!=-1) {
        _tp_list_append(tp, tp_list_val(r), tp_string_slice(tp,v,0,i));
        tp_str_val(v) += i + tp_str_len(d);
        tp_str_len(v) -= i + tp_str_len(d);
/*         tp_grey(tp,r); // should stop gc or something instead */
    }
    _tp_list_append(tp, tp_list_val(r), tp_string_slice(tp, v, 0, tp_str_len(v)));
/*     tp_grey(tp,r); // should stop gc or something instead */
    return r;
}

tp_obj tp_find(tp_vm *tp) {
    tp_obj s = TP_OBJ();
    tp_obj v = TP_OBJ();
    return tp_number(tp, _tp_str_index(s,v));
}

tp_obj tp_str_index(tp_vm *tp) {
    tp_obj s = TP_OBJ();
    tp_obj v = TP_OBJ();
    int n = _tp_str_index(s,v);
    if (n >= 0) { 
        return tp_number(tp, n); 
    }
    tp_raise(None,"tp_str_index(%s,%s)",s,v);
}

tp_obj tp_str2(TP) {
    tp_obj v = TP_OBJ();
    return tp_str(tp,v);
}

tp_obj tp_chr(tp_vm *tp) {
    int v = TP_NUM();
    return tp_string_n(tp, tp->chars[(unsigned char)v],1);
}

tp_obj tp_ord(tp_vm *tp) {
    char *s = TP_STR();
    return tp_number(tp, s[0]);
}

tp_obj tp_strip(tp_vm *tp) {
    char *v = TP_STR();
    int i, l = strlen(v); 
    int a = l, b = 0;
    tp_obj r;
    char *s;
    for (i=0; i<l; i++) {
        if (v[i] != ' ' && v[i] != '\n' && v[i] != '\t' && v[i] != '\r') {
            a = _tp_min(a,i); b = _tp_max(b,i+1);
        }
    }
    if ((b-a) < 0) { 
        return tp_string(tp, ""); 
    }
    r = tp_string_t(tp, b-a);
    s = tp_str_val(r);
    memcpy(s,v+a,b-a);
    return tp_track(tp,r);
}

/* like strstr, but handles embedded '\0's in both strings */
static char *str_find(char *s, int slen, char *toFind, int toFindLen)
{
    char *tmp;
    char *toFindTmp;
    int toFindLenLeft;
    char c;
    if (0 == toFindLen)
        return NULL;
    c = *toFind;
    for (;;) {
        if (toFindLen > slen)
            return NULL;
        if (c != *s++) {
            --slen;
            continue;
        }
        tmp = s;
        toFindLenLeft = toFindLen - 1;
        toFindTmp = toFind + 1;
        while (toFindLenLeft > 0) {
            if (*tmp++ != *toFindTmp++)
                return NULL;
            --toFindLenLeft;
        }
        return s-1;
    }
}

static char *str_replace(char *s, int slen, char *toReplace, int toReplaceLen, char *replaceWith, int replaceWithLen, int *lenOut)
{
    char *pos;
    int toReplaceCount = 0;
    char *result;
    char *prev;
    char *tmp = s;
    int slenLeft;
    for (;;) {
        slenLeft = tmp - s;
        slenLeft = slen - slenLeft; /* yes, 2 statements to avoid arithmetic overflow */
        pos = str_find(tmp, slenLeft, toReplace, toReplaceLen);
        if (!pos) break;
        ++toReplaceCount;
        tmp = pos + toReplaceLen;
    }
    *lenOut = slen + toReplaceCount * (replaceWithLen - toReplaceLen);
    result = (char*)malloc(*lenOut+1);
    tmp = result;
    prev = s;
    slenLeft = slen;
    for (;;) {
        int toCopy;
        pos = str_find(s, slenLeft, toReplace, toReplaceLen);
        if (!pos) {
            memcpy(tmp, s, slenLeft);
            break;
        }
        toCopy = pos - s;
        memcpy(tmp, s, toCopy);
        tmp += toCopy;
        memcpy(tmp, replaceWith, replaceWithLen);
        tmp += replaceWithLen;
        prev = s;
        slenLeft -= toCopy + toReplaceLen;
        s = pos + toReplaceLen;
    }
    result[*lenOut] = 0;
    return result;
}

tp_obj tp_replace(tp_vm *tp) {
    tp_obj s = TP_OBJ();
    tp_obj k = TP_OBJ();
    tp_obj v = TP_OBJ();
    int strlength;
    /* TODO: optimize by not copying strresult */
    char *strresult = str_replace(tp_str_val(s), tp_str_len(s), tp_str_val(k), tp_str_len(k), tp_str_val(v), tp_str_len(v), &strlength);
    tp_obj result = tp_string_t(tp, strlength);
    memcpy(tp_str_val(result), strresult, strlength);
    free(strresult);
    return tp_track(tp, result);
}

