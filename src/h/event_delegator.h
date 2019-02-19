#ifndef d_event_delegator_h
#define d_event_delegator_h

#include "ote/src/h/editor.h"

#include "opl/src/h/input.h"
#include "ovp/src/h/ovp.h"

typedef struct global_state
{
    event e;
    editor **editors;
    u32 editors_size;
    editor *focused_editor;
    
    vec2 mouse_position;

    ovp *config;    
} global_state;

global_state *ctor_global_state(int argc, char **argv, ovp *config);

s32 delegate_event(global_state *gs);

#endif
