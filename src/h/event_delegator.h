#ifndef EVENT_HANDLER_H
#define EVENT_HANDLER_H

#include "../h/editor.h"
#include "../../../OSAL/src/h/input.h"

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
