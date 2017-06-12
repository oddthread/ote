#ifndef ACTION_H
#define ACTION_H

#include "../../../OSAL/src/h/util.h"

typedef struct action
{
    vec2 cursor_position;
    vec2 optional_end_position;
	enum {ACTION_TEXT,ACTION_DELETE,ACTION_BACKSPACE} type;
    char *text;
} action;
action *ctor_action(vec2 cursor_position, vec2 optional_end_position, char *text, u32 type);
void dtor_action(action *a);

#endif
