tp_obj tp_string_t(TP, int n) {
    tp_obj r = tp_string_n(0,n);
    r->string.info = tp_malloc(sizeof(_tp_string)+n);
    r->string.val = r->string.info->s;
    return r;
}

tp_obj tp_printf(TP,char *fmt,...) {
    int l;
    tp_obj r;
    char *s;
    va_list arg;
    va_start(arg, fmt);
    l = vsnprintf(NULL, 0, fmt,arg);
    r = tp_string_t(tp,l);
    s = r->string.val;
    va_end(arg);
    va_start(arg, fmt);
    vsprintf(s,fmt,arg);
    va_end(arg);
    return tp_track(tp,r);
}

int _tp_str_index(tp_obj s, tp_obj k) {
    int i=0;
    while ((s->string.len - i) >= k->string.len) {
        if (memcmp(s->string.val+i,k->string.val,k->string.len) == 0) {
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
    for (i=0; i < val->list.val->len; i++) {
        if (i!=0) { l += delim->string.len; }
        l += tp_str(tp, val->list.val->items[i])->string.len;
    }
    r = tp_string_t(tp,l);
    s = r->string.val;
    l = 0;
    for (i=0; i < val->list.val->len; i++) {
        tp_obj e;
        if (i!=0) {
            memcpy(s+l, delim->string.val, delim->string.len); 
            l += delim->string.len;
        }
        e = tp_str(tp, val->list.val->items[i]);
        memcpy(s+l, e->string.val, e->string.len); 
        l += e->string.len;
    }
    return tp_track(tp,r);
}

tp_obj tp_string_slice(TP,tp_obj s, int a, int b) {
    tp_obj r = tp_string_t(tp,b-a);
    char *m = r->string.val;
    memcpy(m, s->string.val+a,b-a);
    return tp_track(tp,r);
}

tp_obj tp_split(TP) {
    tp_obj v = TP_OBJ();
    tp_obj d = TP_OBJ();
    tp_obj r = tp_list(tp);
    
    int i;
    while ((i=_tp_str_index(v,d))!=-1) {
        _tp_list_append(tp, r->list.val, tp_string_slice(tp,v,0,i));
        v->string.val += i + d->string.len; 
        v->string.len -= i + d->string.len;
//         tp_grey(tp,r); // should stop gc or something instead
    }
    _tp_list_append(tp, r->list.val, tp_string_slice(tp,v,0,v->string.len));
//     tp_grey(tp,r); // should stop gc or something instead
    return r;
}

tp_obj tp_find(TP) {
    tp_obj s = TP_OBJ();
    tp_obj v = TP_OBJ();
    return tp_number(_tp_str_index(s,v));
}

tp_obj tp_str_index(TP) {
    tp_obj s = TP_OBJ();
    tp_obj v = TP_OBJ();
    int n = _tp_str_index(s,v);
    if (n >= 0) { return tp_number(n); }
    tp_raise(None,"tp_str_index(%s,%s)",s,v);
}

tp_obj tp_str2(TP) {
    tp_obj v = TP_OBJ();
    return tp_str(tp,v);
}

tp_obj tp_chr(TP) {
    int v = TP_NUM();
    return tp_string_n(tp->chars[(unsigned char)v],1);
}
tp_obj tp_ord(TP) {
    char *s = TP_STR();
    return tp_number(s[0]);
}

tp_obj tp_strip(TP) {
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
    if ((b-a) < 0) { return tp_string(""); }
    r = tp_string_t(tp, b-a);
    s = r->string.val;
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

tp_obj tp_replace(TP) {
    tp_obj s = TP_OBJ();
    tp_obj k = TP_OBJ();
    tp_obj v = TP_OBJ();
    int strlength;
    char *strresult = str_replace(s->string.val, s->string.len, k->string.val, k->string.len, v->string.val, v->string.len, &strlength);
    tp_obj result = tp_string_n(strresult, strlength);
    return tp_track(tp, result);
}

