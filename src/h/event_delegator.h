#ifndef d_event_delegator_h
#define d_event_delegator_h

#include "ode/src/h/editor.h"

#include "opl/src/h/input.h"

typedef struct global_state
{
    event e;
    editor **editors;
    u32 editors_size;
    editor *focused_editor;
    
    vec2 mouse_position;    
} global_state;

global_state *ctor_global_state(int argc, char **argv);

s32 delegate_event(global_state *gs);

#endif
