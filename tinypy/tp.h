#ifndef TP_H
#define TP_H

#include <setjmp.h>
#include <sys/stat.h>
#ifndef __USE_ISOC99
#define __USE_ISOC99
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <math.h>
#include <assert.h>

#ifdef __GNUC__
#define tp_inline __inline__
#endif

#ifdef _MSC_VER
#define tp_inline __inline
#endif

#ifndef tp_inline
#error "Unsuported compiler"
#endif
#define tp_malloc(x) calloc((x),1)
#define tp_realloc(x,y) realloc(x,y)
#define tp_free(x) free(x)

/* #include <gc/gc.h>
   #define tp_malloc(x) GC_MALLOC(x)
   #define tp_realloc(x,y) GC_REALLOC(x,y)
   #define tp_free(x)*/

typedef enum objtype {
    TP_NONE,TP_NUMBER,TP_STRING,TP_DICT,
    TP_LIST,TP_FNC,TP_DATA,
} objtype;

typedef double tp_num;

/* Pointer tagging scheme:
 2 lower bits reserved for a tag
 00 - small integer (not used yet)
 01 - number (double)
 10 - string (not used yet)
 11 - the rest
*/
#define tp_number_val(o) (o)->number.val
#define tp_str_val(v) (v)->string.val
#define tp_str_len(v) (v)->string.len
#define tp_list_val(v) (v)->list.val

#define tagSmallInt 0
#define tagNumber   1
#define tagString   2
#define tagObject   3

#define tag_from_ptr(v) ((long)v & 3)
#define untag_ptr(v) (tp_obj)((long)v & ~3)
#define tag_ptr(v,tag) (tp_obj)((long)v | tag)

#define is_number(v) tag_from_ptr(v) == tagNumber
#define is_string(v) tag_from_ptr(v) == tagString
#define is_object(v) tag_from_ptr(v) == tagObject

#define tp_number_val(v) *(double*)untag_ptr(v)

typedef struct tp_string_ {
    objtype type;
    struct _tp_string *info;
    char *val;
    int len;
    int gci;
} tp_string_;

typedef struct tp_list_ {
    objtype type;
    struct _tp_list *val;
} tp_list_;

typedef struct tp_dict_ {
    objtype type;
    struct _tp_dict *val;
} tp_dict_;

typedef struct tp_fnc_ {
    objtype type;
    struct _tp_fnc *val;
    int ftype;
    void *fval;
} tp_fnc_;

typedef struct tp_data_ {
    objtype type;
    struct _tp_data *info;
    void *val;
    struct tp_meta *meta;
} tp_data_;

typedef union tp_obj_ {
    objtype type;
    struct { int type; int *data; } gci;
    tp_string_ string;
    tp_dict_ dict;
    tp_list_ list;
    tp_fnc_ fnc;
    tp_data_ data;
} tp_obj_;

typedef tp_obj_* tp_obj;

typedef struct _tp_string {
    int gci;
    char s[1];
} _tp_string;

typedef struct _tp_list {
    int gci;
    tp_obj *items;
    int len;
    int alloc;
} _tp_list;

typedef struct tp_item {
    int used;
    int hash;
    tp_obj key;
    tp_obj val;
} tp_item;

typedef struct _tp_dict {
    int gci;
    tp_item *items;
    int len;
    int alloc;
    int cur;
    int mask;
    int used;
} _tp_dict;

typedef struct _tp_fnc {
    int gci;
    tp_obj self;
    tp_obj globals;
} _tp_fnc;

typedef union tp_code {
    unsigned char i;
    struct { unsigned char i,a,b,c; } regs;
    struct { char val[4]; } string;
    struct { float val; } number;
} tp_code;

typedef struct tp_frame_ {
    tp_code *codes;
    tp_code *cur;
    tp_code *jmp;
    tp_obj *regs;
    tp_obj *ret_dest;
    tp_obj fname;
    tp_obj name;
    tp_obj line;
    tp_obj globals;
    int lineno;
    int cregs;
} tp_frame_;

#define TP_GCMAX 4096
#define TP_FRAMES 256
/* #define TP_REGS_PER_FRAME 256 */
#define TP_REGS 16384
typedef struct tp_vm {
    tp_obj builtins;
    tp_obj modules;
    tp_frame_ frames[TP_FRAMES];
    tp_obj _params;
    tp_obj params;
    tp_obj _regs;
    tp_obj *regs;
    tp_obj root;
    jmp_buf buf;
    int jmp;
    tp_obj ex;
    char chars[256][2];
    int cur;
    /* gc*/
    _tp_list *white;
    _tp_list *grey;
    _tp_list *black;
    _tp_dict *strings;
    int steps;
    int instructions;
} tp_vm;

#define TP tp_vm *tp
typedef struct tp_meta {
    int type;
    tp_obj (*get)(TP,tp_obj,tp_obj);
    void (*set)(TP,tp_obj,tp_obj,tp_obj);
    void (*free)(TP,tp_obj);
/*     tp_obj (*del)(TP,tp_obj,tp_obj);
       tp_obj (*has)(TP,tp_obj,tp_obj);
       tp_obj (*len)(TP,tp_obj);*/
} tp_meta;

typedef struct _tp_data {
    int gci;
    tp_meta meta;
} _tp_data;

/* NOTE: these are the few out of namespace items for convenience*/
#define True tp_number(tp, 1)
#define False tp_number(tp, 0)
#define STR(v) (tp_str_val(tp_str(tp,(v))))
extern tp_obj_ NoneObj;
#define None tag_ptr(&NoneObj, tagObject)

void tp_set(TP,tp_obj,tp_obj,tp_obj);
tp_obj tp_get(TP,tp_obj,tp_obj);
tp_obj tp_len(TP,tp_obj);
tp_obj tp_str(TP,tp_obj);
int tp_cmp(TP,tp_obj,tp_obj);
void _tp_raise(TP,tp_obj);
tp_obj tp_printf(TP,char *fmt,...);
tp_obj tp_track(TP,tp_obj);
void tp_grey(TP,tp_obj);

/* __func__ __VA_ARGS__ __FILE__ __LINE__ */
#define tp_raise(r,fmt,...) { \
    _tp_raise(tp,tp_printf(tp,fmt,__VA_ARGS__)); \
    return r; \
}

tp_inline static objtype obj_type(tp_obj o) {
    if (tag_from_ptr(o) == tagObject)
        return o->type;
    assert(tagNumber == tag_from_ptr(o));
    return TP_NUMBER;
}

#define __params (tp->params)
#define TP_OBJ() (tp_get(tp, tp->params, None))
tp_inline static tp_obj tp_type(TP,int t,tp_obj v) {
    if (obj_type(v) != t) { 
        tp_raise(None, "_tp_type(%d,%s)", t, STR(v)); 
    }
    return v;
}
#define TP_TYPE(t) tp_type(tp,t,TP_OBJ())
#define TP_NUM() (tp_number_val(TP_TYPE(TP_NUMBER)))
#define TP_STR() (STR(TP_TYPE(TP_STRING)))
#define TP_DEFAULT(d) (tp_list_val(__params)->len ? tp_get(tp,__params,None):(d))
#define TP_LOOP(e) \
    int __l = tp_list_val(__params)->len; \
    int __i; for (__i=0; __i<__l; __i++) { \
    (e) = _tp_list_get(tp, tp_list_val(__params), __i, "TP_LOOP");
#define TP_END \
    }

tp_inline static int _tp_min(int a, int b) { return (a<b?a:b); }
tp_inline static int _tp_max(int a, int b) { return (a>b?a:b); }
tp_inline static int _tp_sign(tp_num v) { return (v<0?-1:(v>0?1:0)); }

extern tp_obj obj_alloc(objtype type);
extern tp_obj tp_number(tp_vm *tp, tp_num v);
extern tp_obj tp_string_n(tp_vm *tp, char *v,int n);
extern tp_obj tp_string(tp_vm *tp, char *v);

#endif
