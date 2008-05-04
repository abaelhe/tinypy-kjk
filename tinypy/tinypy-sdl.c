#include "SDL.h"

#include "tinypy.h"


SDL_Surface *s;
tp_obj set_mode(TP) {
    int w = TP_NUM();
    int h = TP_NUM();
    SDL_Init(SDL_INIT_EVERYTHING);
    s = SDL_SetVideoMode(w,h,32,0);
    return None;
}

void real_set_pixel(int x, int y, int c) {
    if (x<s->clip_rect.x || y <s->clip_rect.y || x >= (s->clip_rect.x+s->clip_rect.w) || y >=(s->clip_rect.y+s->clip_rect.h)) { return ; }  
    Uint32 *p = (Uint32*)((Uint8*)s->pixels+y*s->pitch);
    *(p+x) = (Uint32)c;
}

tp_obj set_pixel(TP) {
    int x = TP_NUM();
    int y = TP_NUM();
    int c = TP_NUM();
    
    real_set_pixel(x,y,c);
    return None;
}


tp_obj update(TP) {
    int x = TP_NUM();
    int y = TP_NUM();
    int w = TP_NUM();
    int h = TP_NUM();
    SDL_Rect r;
    r.x=x; r.y=y;
    r.w=w; r.h=h;
    SDL_UpdateRects(s, 1, &r);
    return None;
}

tp_obj get_ticks(tp_vm *tp) {
    return tp_number(tp, SDL_GetTicks());
}

tp_obj _sdl_event_get(tp_vm *tp, tp_obj self, tp_obj k) {
    SDL_Event *e = self->data.val;
    char *key = STR(k);
    if (strcmp(key,"type")==0) { return tp_number(tp, e->type); }
    return None;
}

tp_obj gfx_get_event(TP) {
    SDL_Event *e = malloc(sizeof(SDL_Event));
    if (SDL_PollEvent(e)) {
        tp_obj r = tp_data(tp,e);
        r->data.meta->get = _sdl_event_get;
        return r;
    }
    return None;
}

tp_obj get_mouse_pos(tp_vm *tp) {
    int x,y,b;
    b = SDL_GetMouseState(&x,&y);
    tp_obj r;
    r = tp_dict_n(tp,3,(tp_obj[]){
        tp_string(tp, "x"),tp_number(tp, x),
        tp_string(tp, "y"),tp_number(tp, y),
        tp_string(tp, "b"),tp_number(tp, b)
        });
    return r;
}

void sdl_init(tp_vm *tp) {
    tp_obj context = tp->builtins;
    tp_set(tp,context,tp_string(tp, "set_mode"),tp_fnc(tp,set_mode));
    tp_set(tp,context,tp_string(tp, "set_pixel"),tp_fnc(tp,set_pixel));
    tp_set(tp,context,tp_string(tp, "update"),tp_fnc(tp,update));
    tp_set(tp,context,tp_string(tp, "get_ticks"),tp_fnc(tp,get_ticks));
    tp_set(tp,context,tp_string(tp, "gfx_get_event"),tp_fnc(tp,gfx_get_event));
    tp_set(tp,context,tp_string(tp, "get_mouse_pos"),tp_fnc(tp,get_mouse_pos));
}

int main(int argc, char *argv[]) {
    tp_vm *tp = tp_init(argc,argv);
    sdl_init(tp);
    tp_call(tp,"py2bc","tinypy",None);
    tp_deinit(tp);
    return(0);
}

