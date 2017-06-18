#include "../h/editor.h"
#include "../h/action.h"

action *ctor_action(vec2 cursor_position, vec2 optional_end_position, char *text, u32 type)
{
    action *a=malloc(sizeof(action));
    a->cursor_position=cursor_position;
    a->optional_end_position=optional_end_position;
    a->text=text;
    a->type=type;
    return a;
}
void dtor_action(action *a)
{
    free(a->text);
    free(a);
}