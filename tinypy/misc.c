
tp_obj *tp_ptr(tp_obj o) {
    tp_obj *ptr = tp_malloc(sizeof(tp_obj)); 
    *ptr = o;
    return ptr;
}

tp_obj _tp_dcall(tp_vm *tp, tp_obj fnc(tp_vm *)) { 
    return fnc(tp);
}

tp_obj _tp_tcall(tp_vm *tp, tp_obj fnc) {
    if (tp_fnc_ftype(fnc) & 2) {
        _tp_list_insert(tp, tp_list_val(tp->params), 0, tp_fnc_val(fnc)->self);
    }
    
    return _tp_dcall(tp, tp_fnc_fval(fnc));
}

tp_obj tp_fnc_new(tp_vm *tp, int t, void *v, tp_obj s, tp_obj g) {
    tp_obj r = obj_alloc(TP_FNC);
    _tp_fnc *self = tp_malloc(sizeof(_tp_fnc));
    self->self = s;
    self->globals = g;
    tp_fnc_ftype(r) = t;
    tp_fnc_val(r) = self;
    tp_fnc_fval(r) = v;
    return tp_track(tp, r);
}

tp_obj tp_def(TP, void *v, tp_obj g) {
    return tp_fnc_new(tp,1,v,None,g);
}

tp_obj tp_fnc(TP, tp_obj v(TP)) {
    return tp_fnc_new(tp,0,v,None,None);
}

tp_obj tp_method(TP, tp_obj self,tp_obj v(TP)) {
    return tp_fnc_new(tp,2,v,self,None);
}

tp_obj tp_data(tp_vm *tp, void *v) {
    tp_obj r = obj_alloc(TP_DATA);
    tp_data_ *d = &r->data;
    d->info = tp_malloc(sizeof(_tp_data));
    d->val = v;
    d->meta = &d->info->meta;
    return tp_track(tp,r);
}

tp_obj tp_params(tp_vm *tp) {
    tp_obj r;
    tp->params = tp_list_val(tp->_params)->items[tp->cur];
    r = tp_list_val(tp->_params)->items[tp->cur];
    tp_list_val(r)->len = 0;
    return r;
}

tp_obj tp_params_n(TP,int n, tp_obj argv[]) {
    tp_obj r = tp_params(tp);
    int i; 
    for (i=0; i<n; i++) { 
        _tp_list_append(tp, tp_list_val(r), argv[i]);
    }
    return r;
}

tp_obj tp_params_v(TP,int n,...) {
    int i;
    tp_obj r = tp_params(tp);
    va_list a; va_start(a,n);
    for (i=0; i<n; i++) { 
        _tp_list_append(tp, tp_list_val(r), va_arg(a,tp_obj));
    }
    va_end(a);
    return r;
}
